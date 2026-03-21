#include "requests_cpp/proxy_tunnel.hpp"
#include "requests_cpp/exceptions.hpp"
#include <cstring>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#define ssize_t int
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace requests_cpp {

// SocketConnection类的简单实现（这里简化处理，实际项目中应该有完整的实现）
class SocketConnection {
public:
    SocketConnection(int socket_fd) : socket_fd_(socket_fd) {}
    
    ~SocketConnection() {
        if (socket_fd_ != -1) {
            close(socket_fd_);
        }
    }
    
    int get_socket() const { return socket_fd_; }
    
    bool send_data(const void* data, size_t length) {
        const char* ptr = static_cast<const char*>(data);
        size_t sent = 0;
        
        while (sent < length) {
            ssize_t result = ::send(socket_fd_, ptr + sent, length - sent, 0);
            if (result <= 0) {
                return false;
            }
            sent += result;
        }
        return true;
    }
    
    bool receive_data(void* buffer, size_t length) {
        char* ptr = static_cast<char*>(buffer);
        size_t received = 0;
        
        while (received < length) {
            ssize_t result = recv(socket_fd_, ptr + received, length - received, 0);
            if (result <= 0) {
                return false;
            }
            received += result;
        }
        return true;
    }
    
    // 接收指定长度的数据，但不阻塞等待全部接收
    bool receive_partial(void* buffer, size_t max_length, size_t* received_length) {
        int recv_flags = 0;
#ifndef _WIN32
        recv_flags = MSG_DONTWAIT;
#endif
        ssize_t result = recv(socket_fd_, static_cast<char*>(buffer), static_cast<int>(max_length), recv_flags);
        if (result <= 0) {
            return false;
        }
        *received_length = static_cast<size_t>(result);
        return true;
    }

private:
    int socket_fd_;
};

void SocketConnectionDeleter::operator()(SocketConnection* connection) const {
    delete connection;
}

// 辅助函数：建立TCP连接
SocketConnectionPtr connect_to_host(const std::string& host, int port) {
    struct addrinfo hints, *result, *current;
    int sock;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // 支持IPv4和IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    std::string port_str = std::to_string(port);
    int status = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (status != 0) {
        return nullptr;
    }
    
    // 尝试连接到每一个地址直到成功
    for (current = result; current != nullptr; current = current->ai_next) {
        sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (sock == -1) {
            continue;
        }
        
        if (::connect(sock, current->ai_addr, current->ai_addrlen) == 0) {
            freeaddrinfo(result);
            return SocketConnectionPtr(new SocketConnection(sock));
        }
        
        close(sock);
    }
    
    freeaddrinfo(result);
    return nullptr;
}

// CONNECT隧道实现
SocketConnectionPtr ConnectTunnel::establish(
    const std::string& proxy_host,
    int proxy_port,
    const std::string& dest_host,
    int dest_port) {
    
    auto connection = connect_to_host(proxy_host, proxy_port);
    if (!connection) {
        return nullptr;
    }
    
    if (!send_connect_request(*connection, dest_host, dest_port)) {
        return nullptr;
    }
    
    if (!read_connect_response(*connection)) {
        return nullptr;
    }
    
    return connection;
}

bool ConnectTunnel::send_connect_request(
    SocketConnection& connection,
    const std::string& dest_host,
    int dest_port) {
    
    std::ostringstream oss;
    oss << "CONNECT " << dest_host << ":" << dest_port << " HTTP/1.1\r\n";
    oss << "Host: " << dest_host << ":" << dest_port << "\r\n";
    oss << "Proxy-Connection: Keep-Alive\r\n";
    oss << "\r\n";
    
    std::string request = oss.str();
    return connection.send_data(request.c_str(), request.length());
}

bool ConnectTunnel::read_connect_response(SocketConnection& connection) {
    char buffer[1024];
    size_t received = 0;
    
    // 读取响应头部
    bool found_end = false;
    std::string response;
    
    while (!found_end && response.length() < sizeof(buffer) - 1) {
        size_t bytes_received;
        if (!connection.receive_partial(buffer, sizeof(buffer) - 1 - response.length(), &bytes_received)) {
            // 如果非阻塞接收失败，尝试阻塞接收
            if (recv(connection.get_socket(), buffer, 1, 0) <= 0) {
                return false;
            }
            response += buffer[0];
        } else {
            buffer[bytes_received] = '\0';
            response += std::string(buffer, bytes_received);
        }
        
        // 检查是否收到完整的HTTP头部结束标记
        if (response.find("\r\n\r\n") != std::string::npos) {
            found_end = true;
        } else if (response.find("\n\n") != std::string::npos) {
            found_end = true;
        }
    }
    
    // 解析状态码
    size_t first_space = response.find(' ');
    if (first_space == std::string::npos) {
        return false;
    }
    
    size_t second_space = response.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        return false;
    }
    
    std::string status_code_str = response.substr(first_space + 1, second_space - first_space - 1);
    int status_code = std::stoi(status_code_str);
    
    // 2xx 状态码表示成功
    return status_code >= 200 && status_code < 300;
}

// SOCKS4实现
SocketConnectionPtr Socks4Client::connect(
    const std::string& proxy_host,
    int proxy_port,
    const std::string& dest_host,
    int dest_port,
    const std::string& user_id) {
    
    auto connection = connect_to_host(proxy_host, proxy_port);
    if (!connection) {
        return nullptr;
    }
    
    if (!send_request(*connection, dest_host, dest_port, user_id)) {
        return nullptr;
    }
    
    if (!read_response(*connection)) {
        return nullptr;
    }
    
    return connection;
}

bool Socks4Client::send_request(
    SocketConnection& connection,
    const std::string& dest_host,
    int dest_port,
    const std::string& user_id) {
    
    // 首先尝试解析为IP地址
    struct in_addr addr;
    if (inet_pton(AF_INET, dest_host.c_str(), &addr) != 1) {
        // 如果不是有效的IP地址，则需要DNS解析
        struct addrinfo hints, *result;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;  // IPv4
        
        if (getaddrinfo(dest_host.c_str(), nullptr, &hints, &result) != 0) {
            return false;
        }
        
        addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
        freeaddrinfo(result);
    }
    
    // 构建SOCKS4请求
    uint8_t request[1024];  // 足够大的缓冲区
    size_t pos = 0;
    
    // 版本号 (4)
    request[pos++] = 4;
    // 命令码 (1 = CONNECT)
    request[pos++] = 1;
    // 端口号 (2字节)
    request[pos++] = (dest_port >> 8) & 0xFF;
    request[pos++] = dest_port & 0xFF;
    // IP地址 (4字节)
    memcpy(&request[pos], &addr, 4);
    pos += 4;
    
    // 用户ID (以null结尾)
    for (char c : user_id) {
        request[pos++] = static_cast<uint8_t>(c);
    }
    request[pos++] = 0;  // null终止符
    
    // 目标主机名 (对于SOCKS4，如果IP解析失败，可以传递主机名)
    // 这里我们已经解析了IP，所以不需要额外的主机名字段
    
    return connection.send_data(request, pos);
}

bool Socks4Client::read_response(SocketConnection& connection) {
    uint8_t response[8];
    if (!connection.receive_data(response, 8)) {
        return false;
    }
    
    // 检查版本号
    if (response[0] != 0) {
        return false;
    }
    
    // 检查状态码
    // 90 = 请求被接受
    // 91 = 请求被拒绝或无法连接
    // 92 = 无法连接到客户端标识（identd未运行）
    // 93 = 客户端标识不匹配
    return response[1] == 90;
}

// SOCKS5实现
SocketConnectionPtr Socks5Client::connect(
    const std::string& proxy_host,
    int proxy_port,
    const std::string& dest_host,
    int dest_port,
    const std::string& username,
    const std::string& password) {
    
    auto connection = connect_to_host(proxy_host, proxy_port);
    if (!connection) {
        return nullptr;
    }
    
    if (!handshake(*connection, username, password)) {
        return nullptr;
    }
    
    if (!send_request(*connection, dest_host, dest_port)) {
        return nullptr;
    }
    
    if (!read_response(*connection)) {
        return nullptr;
    }
    
    return connection;
}

bool Socks5Client::handshake(
    SocketConnection& connection,
    const std::string& username,
    const std::string& password) {
    
    // 发送认证方法协商请求
    uint8_t auth_methods[255];
    size_t pos = 0;
    
    // 版本号 (5)
    auth_methods[pos++] = 5;
    
    // 认证方法数量
    if (username.empty() || password.empty()) {
        // 不需要用户名密码认证，只支持无认证
        auth_methods[pos++] = 1;  // 只有一种方法
        auth_methods[pos++] = 0;  // 无认证
    } else {
        // 支持用户名密码认证和无认证
        auth_methods[pos++] = 2;  // 两种方法
        auth_methods[pos++] = 2;  // 用户名密码认证
        auth_methods[pos++] = 0;  // 无认证
    }
    
    if (!connection.send_data(auth_methods, pos)) {
        return false;
    }
    
    // 读取认证方法选择
    uint8_t selection[2];
    if (!connection.receive_data(selection, 2)) {
        return false;
    }
    
    if (selection[0] != 5) {  // 检查版本号
        return false;
    }
    
    uint8_t auth_method = selection[1];
    
    // 根据选择的认证方法进行处理
    if (auth_method == 0) {
        // 无认证
        return true;
    } else if (auth_method == 2 && !username.empty() && !password.empty()) {
        // 用户名密码认证
        return perform_username_password_authentication(connection, username, password);
    } else {
        // 不支持的认证方法
        return false;
    }
}

bool Socks5Client::perform_username_password_authentication(
    SocketConnection& connection,
    const std::string& username,
    const std::string& password) {
    
    // 构建用户名密码认证请求
    uint8_t auth_request[512];  // 足够大的缓冲区
    size_t pos = 0;
    
    // 版本号 (1)
    auth_request[pos++] = 1;
    // 用户名长度
    auth_request[pos++] = static_cast<uint8_t>(username.length());
    // 用户名
    for (char c : username) {
        auth_request[pos++] = static_cast<uint8_t>(c);
    }
    // 密码长度
    auth_request[pos++] = static_cast<uint8_t>(password.length());
    // 密码
    for (char c : password) {
        auth_request[pos++] = static_cast<uint8_t>(c);
    }
    
    if (!connection.send_data(auth_request, pos)) {
        return false;
    }
    
    // 读取认证响应
    uint8_t auth_response[2];
    if (!connection.receive_data(auth_response, 2)) {
        return false;
    }
    
    // 检查版本号和状态
    if (auth_response[0] != 1 || auth_response[1] != 0) {
        return false;  // 认证失败
    }
    
    return true;
}

bool Socks5Client::send_request(
    SocketConnection& connection,
    const std::string& dest_host,
    int dest_port) {
    
    // 构建SOCKS5连接请求
    uint8_t request[256];  // 足够大的缓冲区
    size_t pos = 0;
    
    // 版本号 (5)
    request[pos++] = 5;
    // 命令码 (1 = CONNECT)
    request[pos++] = 1;
    // 保留字段 (必须为0)
    request[pos++] = 0;
    
    // 目标地址类型和地址
    // 首先尝试解析为IP地址
    struct in_addr addr;
    if (inet_pton(AF_INET, dest_host.c_str(), &addr) == 1) {
        // 是有效的IP地址
        request[pos++] = 1;  // IPv4地址
        memcpy(&request[pos], &addr, 4);
        pos += 4;
    } else {
        // 不是IP地址，使用域名
        request[pos++] = 3;  // 域名
        request[pos++] = static_cast<uint8_t>(dest_host.length());  // 域名长度
        for (char c : dest_host) {
            request[pos++] = static_cast<uint8_t>(c);
        }
    }
    
    // 端口号 (2字节)
    request[pos++] = (dest_port >> 8) & 0xFF;
    request[pos++] = dest_port & 0xFF;
    
    return connection.send_data(request, pos);
}

bool Socks5Client::read_response(SocketConnection& connection) {
    uint8_t response[262];  // 足够大的缓冲区以容纳可能的域名响应
    // 先读取固定部分
    if (!connection.receive_data(response, 4)) {
        return false;
    }
    
    // 检查版本号
    if (response[0] != 5) {
        return false;
    }
    
    // 检查结果码 (0 = 成功)
    if (response[1] != 0) {
        return false;
    }
    
    // 跳过保留字段 (response[2])
    
    // 获取地址类型
    uint8_t addr_type = response[3];
    size_t addr_len = 0;
    
    switch (addr_type) {
        case 1:  // IPv4
            addr_len = 4;
            break;
        case 3:  // 域名
            // 先读取域名长度
            uint8_t domain_len;
            if (!connection.receive_data(&domain_len, 1)) {
                return false;
            }
            addr_len = domain_len;
            break;
        case 4:  // IPv6
            addr_len = 16;
            break;
        default:
            return false;
    }
    
    // 读取地址
    if (!connection.receive_data(response + 4, addr_len + 1)) {  // +1 for port
        return false;
    }
    
    return true;
}

std::vector<uint8_t> Socks5Client::resolve_hostname(const std::string& hostname) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // IPv4
    
    if (getaddrinfo(hostname.c_str(), nullptr, &hints, &result) != 0) {
        // 如果解析失败，返回空向量
        return std::vector<uint8_t>();
    }
    
    struct in_addr addr = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr;
    std::vector<uint8_t> ip_bytes(4);
    memcpy(ip_bytes.data(), &addr, 4);
    
    freeaddrinfo(result);
    return ip_bytes;
}

// 代理隧道工厂实现
SocketConnectionPtr ProxyTunnelFactory::create_tunnel(
    const std::string& proxy_url,
    const std::string& dest_host,
    int dest_port,
    const std::string& username,
    const std::string& password) {
    
    // 解析代理URL
    std::string protocol, host, port_str;
    int port = 0;
    
    size_t proto_end = proxy_url.find("://");
    if (proto_end == std::string::npos) {
        throw RequestException(ErrorCode::Protocol, "Invalid proxy URL format: missing protocol");
    }
    
    protocol = proxy_url.substr(0, proto_end);
    std::string address_part = proxy_url.substr(proto_end + 3);
    
    size_t port_start = address_part.find(':');
    if (port_start != std::string::npos) {
        host = address_part.substr(0, port_start);
        port_str = address_part.substr(port_start + 1);
        try {
            port = std::stoi(port_str);
        } catch (...) {
            throw RequestException(ErrorCode::Protocol, "Invalid proxy port: " + port_str);
        }
    } else {
        host = address_part;
        // 根据协议设置默认端口
        if (protocol == "http" || protocol == "https") {
            port = 80;
        } else if (protocol == "socks5") {
            port = 1080;
        } else if (protocol == "socks4") {
            port = 1080;
        } else {
            port = 8080;  // 默认端口
        }
    }
    
    // 根据协议类型选择适当的隧道实现
    if (protocol == "http" || protocol == "https") {
        // 使用CONNECT隧道
        return ConnectTunnel::establish(host, port, dest_host, dest_port);
    } else if (protocol == "socks5") {
        // 使用SOCKS5
        return Socks5Client::connect(host, port, dest_host, dest_port, username, password);
    } else if (protocol == "socks4") {
        // 使用SOCKS4
        return Socks4Client::connect(host, port, dest_host, dest_port);
    } else {
        throw RequestException(ErrorCode::Protocol, "Unsupported proxy protocol: " + protocol);
    }
}

}  // namespace requests_cpp