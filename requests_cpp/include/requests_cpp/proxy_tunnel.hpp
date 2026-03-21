#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace requests_cpp {

// Forward declarations
class SocketConnection;

// 自定义删除器：让头文件只前置声明类型，也能安全销毁 unique_ptr。
struct SocketConnectionDeleter {
    void operator()(SocketConnection* connection) const;
};

using SocketConnectionPtr = std::unique_ptr<SocketConnection, SocketConnectionDeleter>;

/**
 * CONNECT隧道管理器
 * 处理通过HTTP代理建立HTTPS连接的CONNECT方法
 */
class ConnectTunnel {
public:
    /**
     * 通过HTTP代理建立CONNECT隧道
     * @param proxy_host 代理服务器主机名
     * @param proxy_port 代理服务器端口
     * @param dest_host 目标服务器主机名
     * @param dest_port 目标服务器端口
     * @return 成功建立的隧道连接，失败返回nullptr
     */
    static SocketConnectionPtr establish(
        const std::string& proxy_host,
        int proxy_port,
        const std::string& dest_host,
        int dest_port);

private:
    /**
     * 发送CONNECT请求到代理服务器
     * @param connection 与代理服务器的连接
     * @param dest_host 目标主机
     * @param dest_port 目标端口
     * @return 是否成功发送请求
     */
    static bool send_connect_request(
        SocketConnection& connection,
        const std::string& dest_host,
        int dest_port);

    /**
     * 读取并解析代理服务器响应
     * @param connection 与代理服务器的连接
     * @return 是否成功建立隧道
     */
    static bool read_connect_response(SocketConnection& connection);
};

/**
 * SOCKS协议版本枚举
 */
enum class SocksVersion {
    UNKNOWN = 0,
    SOCKS4 = 4,
    SOCKS5 = 5
};

/**
 * SOCKS4协议实现
 */
class Socks4Client {
public:
    /**
     * 建立SOCKS4连接
     * @param proxy_host 代理服务器主机名
     * @param proxy_port 代理服务器端口
     * @param dest_host 目标服务器主机名
     * @param dest_port 目标服务器端口
     * @param user_id 用户ID（可选）
     * @return 成功建立的连接，失败返回nullptr
     */
    static SocketConnectionPtr connect(
        const std::string& proxy_host,
        int proxy_port,
        const std::string& dest_host,
        int dest_port,
        const std::string& user_id = "");

private:
    /**
     * 发送SOCKS4连接请求
     * @param connection 与代理服务器的连接
     * @param dest_host 目标主机
     * @param dest_port 目标端口
     * @param user_id 用户ID
     * @return 是否成功发送请求
     */
    static bool send_request(
        SocketConnection& connection,
        const std::string& dest_host,
        int dest_port,
        const std::string& user_id);

    /**
     * 读取并解析SOCKS4响应
     * @param connection 与代理服务器的连接
     * @return 是否成功建立连接
     */
    static bool read_response(SocketConnection& connection);
};

/**
 * SOCKS5协议实现
 */
class Socks5Client {
public:
    /**
     * 建立SOCKS5连接
     * @param proxy_host 代理服务器主机名
     * @param proxy_port 代理服务器端口
     * @param dest_host 目标服务器主机名
     * @param dest_port 目标服务器端口
     * @param username 用户名（可选，用于认证）
     * @param password 密码（可选，用于认证）
     * @return 成功建立的连接，失败返回nullptr
     */
    static SocketConnectionPtr connect(
        const std::string& proxy_host,
        int proxy_port,
        const std::string& dest_host,
        int dest_port,
        const std::string& username = "",
        const std::string& password = "");

private:
    /**
     * 执行SOCKS5握手过程（包括认证）
     * @param connection 与代理服务器的连接
     * @param username 用户名
     * @param password 密码
     * @return 是否成功完成握手
     */
    static bool handshake(
        SocketConnection& connection,
        const std::string& username,
        const std::string& password);

    /**
     * 发送SOCKS5连接请求
     * @param connection 与代理服务器的连接
     * @param dest_host 目标主机
     * @param dest_port 目标端口
     * @return 是否成功发送请求
     */
    static bool send_request(
        SocketConnection& connection,
        const std::string& dest_host,
        int dest_port);

    /**
     * 读取并解析SOCKS5响应
     * @param connection 与代理服务器的连接
     * @return 是否成功建立连接
     */
    static bool read_response(SocketConnection& connection);

    /**
     * 执行用户名密码认证
     * @param connection 与代理服务器的连接
     * @param username 用户名
     * @param password 密码
     * @return 是否认证成功
     */
    static bool perform_username_password_authentication(
        SocketConnection& connection,
        const std::string& username,
        const std::string& password);

    /**
     * 获取目标主机的IP地址
     * @param hostname 主机名
     * @return IP地址字节序列
     */
    static std::vector<uint8_t> resolve_hostname(const std::string& hostname);
};

/**
 * 代理隧道工厂类
 * 根据代理URL自动选择合适的隧道协议
 */
class ProxyTunnelFactory {
public:
    /**
     * 根据代理URL建立隧道连接
     * @param proxy_url 代理URL（如 http://proxy:8080, socks5://proxy:1080）
     * @param dest_host 目标主机
     * @param dest_port 目标端口
     * @param username 用户名（用于认证）
     * @param password 密码（用于认证）
     * @return 建立的隧道连接
     */
    static SocketConnectionPtr create_tunnel(
        const std::string& proxy_url,
        const std::string& dest_host,
        int dest_port,
        const std::string& username = "",
        const std::string& password = "");
};

}  // namespace requests_cpp