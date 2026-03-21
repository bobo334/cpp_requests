#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "requests_cpp/connection_pool.hpp"
#include "requests_cpp/exceptions.hpp"
#include "requests_cpp/types.hpp"
#include "requests_cpp/url.hpp"

namespace requests_cpp::http1 {

#if defined(_WIN32)
using SocketHandle = std::uintptr_t;
static constexpr SocketHandle kInvalidSocket = static_cast<SocketHandle>(~0ull);
#else
using SocketHandle = int;
static constexpr SocketHandle kInvalidSocket = -1;
#endif

class Connection {
public:
    Connection() = default;
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&& other) noexcept;
    Connection& operator=(Connection&& other) noexcept;

    bool open(const Url& url);
    bool open(const Url& url, const Timeout& timeout);
    bool is_open() const noexcept { return socket_ != kInvalidSocket; }
    void close();

    std::size_t send_all(const std::string& data);
    std::string recv_all();
    std::string recv_all(const Timeout& timeout);
    std::string recv_some(const Timeout& timeout, std::size_t max_bytes = 4096);

    std::string key() const;
    const std::string& scheme() const noexcept { return scheme_; }
    const std::string& host() const noexcept { return host_; }
    std::uint16_t port() const noexcept { return port_; }

private:
    SocketHandle socket_{kInvalidSocket};
    std::string scheme_{};
    std::string host_{};
    std::uint16_t port_{0};
};

class ConnectionPool {
public:
    explicit ConnectionPool(ConnectionPoolConfig config = {});

    void set_config(ConnectionPoolConfig config);

    Connection acquire(const Url& url);
    Connection acquire(const Url& url, const Timeout& timeout);
    void release(Connection&& connection, bool keep_alive);

private:
    struct PoolEntry {
        Connection connection;
        std::chrono::steady_clock::time_point last_used;
    };

    void prune_idle(bool force);

    ConnectionPoolConfig config_{};
    std::unordered_map<std::string, std::vector<PoolEntry>> pool_{};
    std::size_t total_connections_{0};
    std::chrono::steady_clock::time_point last_health_check_{};
};

}  // namespace requests_cpp::http1
