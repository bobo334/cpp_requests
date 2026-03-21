#include "connection_pool.hpp"

#include "requests_cpp/exceptions.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace requests_cpp::http1 {

namespace {
#if defined(_WIN32)
class WsaGuard {
public:
    WsaGuard() {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw ConnectionError(ErrorCode::Network, "WSAStartup failed", WSAGetLastError());
        }
    }

    ~WsaGuard() { WSACleanup(); }
};
#endif

std::uint16_t default_port(const Url& url) {
    if (url.has_port) {
        return url.port;
    }
    return static_cast<std::uint16_t>(url.scheme == "https" ? 443 : 80);
}

std::string make_key(const std::string& scheme, const std::string& host, std::uint16_t port) {
    std::ostringstream oss;
    oss << scheme << "://" << host << ":" << port;
    return oss.str();
}

}  // namespace

Connection::~Connection() {
    close();
}

ConnectionPool::ConnectionPool(ConnectionPoolConfig config)
    : config_(config), last_health_check_(std::chrono::steady_clock::now()) {}

void ConnectionPool::set_config(ConnectionPoolConfig config) {
    config_ = config;
}

Connection::Connection(Connection&& other) noexcept
    : socket_(other.socket_), scheme_(std::move(other.scheme_)), host_(std::move(other.host_)), port_(other.port_) {
    other.socket_ = kInvalidSocket;
    other.port_ = 0;
}

Connection& Connection::operator=(Connection&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = other.socket_;
        scheme_ = std::move(other.scheme_);
        host_ = std::move(other.host_);
        port_ = other.port_;
        other.socket_ = kInvalidSocket;
        other.port_ = 0;
    }
    return *this;
}

bool Connection::open(const Url& url) {
    Timeout timeout{};
    return open(url, timeout);
}

bool Connection::open(const Url& url, const Timeout& timeout) {
    close();

#if defined(_WIN32)
    static WsaGuard guard;
#endif

    scheme_ = url.scheme;
    host_ = url.host;
    port_ = default_port(url);

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const std::string port_str = std::to_string(port_);
    if (getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &result) != 0) {
#if defined(_WIN32)
        throw ConnectionError(ErrorCode::Network, "getaddrinfo failed", WSAGetLastError());
#else
        throw ConnectionError(ErrorCode::Network, "getaddrinfo failed", errno);
#endif
    }

    SocketHandle sock = kInvalidSocket;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        sock = static_cast<SocketHandle>(::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol));
        if (sock == kInvalidSocket) {
            continue;
        }

        if (timeout.connect_ms > 0) {
#if defined(_WIN32)
            DWORD timeout_ms = static_cast<DWORD>(timeout.connect_ms);
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
#else
            struct timeval tv;
            tv.tv_sec = timeout.connect_ms / 1000;
            tv.tv_usec = (timeout.connect_ms % 1000) * 1000;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
        }

        if (::connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
            break;
        }
#if defined(_WIN32)
        closesocket(sock);
#else
        ::close(sock);
#endif
        sock = kInvalidSocket;
    }

    freeaddrinfo(result);
    if (sock == kInvalidSocket) {
#if defined(_WIN32)
        throw ConnectionError(ErrorCode::Network, "connect failed", WSAGetLastError());
#else
        throw ConnectionError(ErrorCode::Network, "connect failed", errno);
#endif
    }

    socket_ = sock;
    return true;
}

void Connection::close() {
    if (socket_ == kInvalidSocket) {
        return;
    }
#if defined(_WIN32)
    closesocket(socket_);
#else
    ::close(socket_);
#endif
    socket_ = kInvalidSocket;
}

std::string Connection::key() const {
    return make_key(scheme_, host_, port_);
}

std::size_t Connection::send_all(const std::string& data) {
    if (socket_ == kInvalidSocket) {
        return 0;
    }
    std::size_t total = 0;
    const char* buffer = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
#if defined(_WIN32)
        int sent = ::send(socket_, buffer + total, static_cast<int>(remaining), 0);
#else
        ssize_t sent = ::send(socket_, buffer + total, remaining, 0);
#endif
        if (sent <= 0) {
            break;
        }
        total += static_cast<std::size_t>(sent);
        remaining -= static_cast<std::size_t>(sent);
    }
    return total;
}

std::string Connection::recv_all() {
    Timeout timeout{};
    return recv_all(timeout);
}

std::string Connection::recv_all(const Timeout& timeout) {
    std::string data;
    while (true) {
        auto chunk = recv_some(timeout);
        if (chunk.empty()) {
            break;
        }
        data += chunk;
    }
    return data;
}

std::string Connection::recv_some(const Timeout& timeout, std::size_t max_bytes) {
    std::string data;
    if (socket_ == kInvalidSocket) {
        return data;
    }

    if (timeout.read_ms > 0) {
#if defined(_WIN32)
        DWORD timeout_ms = static_cast<DWORD>(timeout.read_ms);
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));
#else
        struct timeval tv;
        tv.tv_sec = timeout.read_ms / 1000;
        tv.tv_usec = (timeout.read_ms % 1000) * 1000;
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    }

    data.resize(max_bytes);
#if defined(_WIN32)
    int received = ::recv(socket_, data.data(), static_cast<int>(max_bytes), 0);
#else
    ssize_t received = ::recv(socket_, data.data(), max_bytes, 0);
#endif
    if (received <= 0) {
        return {};
    }
    data.resize(static_cast<std::size_t>(received));
    return data;
}

void ConnectionPool::prune_idle(bool force) {
    if (config_.health_check_interval_ms <= 0 && !force) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!force && config_.health_check_interval_ms > 0) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_health_check_).count();
        if (elapsed < config_.health_check_interval_ms) {
            return;
        }
    }
    last_health_check_ = now;

    if (config_.idle_timeout_ms <= 0) {
        return;
    }

    for (auto it = pool_.begin(); it != pool_.end();) {
        auto& entries = it->second;
        entries.erase(std::remove_if(entries.begin(), entries.end(), [&](PoolEntry& entry) {
            const auto idle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.last_used).count();
            if (idle_ms >= config_.idle_timeout_ms) {
                entry.connection.close();
                if (total_connections_ > 0) {
                    --total_connections_;
                }
                return true;
            }
            return false;
        }), entries.end());
        if (entries.empty()) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }
}

Connection ConnectionPool::acquire(const Url& url) {
    return acquire(url, Timeout{});
}

Connection ConnectionPool::acquire(const Url& url, const Timeout& timeout) {
    prune_idle(false);

    const std::string key = make_key(url.scheme, url.host, url.has_port ? url.port : default_port(url));
    auto it = pool_.find(key);
    if (it != pool_.end()) {
        auto& entries = it->second;
        while (!entries.empty()) {
            PoolEntry entry = std::move(entries.back());
            entries.pop_back();
            if (total_connections_ > 0) {
                --total_connections_;
            }
            if (entry.connection.is_open()) {
                if (entries.empty()) {
                    pool_.erase(it);
                }
                return std::move(entry.connection);
            }
        }
        pool_.erase(it);
    }

    Connection connection;
    connection.open(url, timeout);
    return connection;
}

void ConnectionPool::release(Connection&& connection, bool keep_alive) {
    if (!connection.is_open()) {
        return;
    }
    if (!keep_alive) {
        connection.close();
        return;
    }

    prune_idle(false);

    if (config_.max_connections > 0 && total_connections_ >= config_.max_connections) {
        connection.close();
        return;
    }

    const std::string key = connection.key();
    auto& entries = pool_[key];
    if (config_.max_connections_per_host > 0 && entries.size() >= config_.max_connections_per_host) {
        connection.close();
        return;
    }

    entries.push_back(PoolEntry{std::move(connection), std::chrono::steady_clock::now()});
    ++total_connections_;
}

}  // namespace requests_cpp::http1
