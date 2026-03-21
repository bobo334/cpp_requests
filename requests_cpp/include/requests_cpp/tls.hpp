#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "requests_cpp/export.hpp"

namespace requests_cpp::tls {

struct REQUESTS_CPP_API TlsConfig {
    bool verify{true};
    std::string ca_path{};
    long connect_timeout_ms{0};
    long read_timeout_ms{0};
    long write_timeout_ms{0};
};

class REQUESTS_CPP_API TlsClient {
public:
    TlsClient();
    ~TlsClient();

    TlsClient(const TlsClient&) = delete;
    TlsClient& operator=(const TlsClient&) = delete;
    TlsClient(TlsClient&&) noexcept;
    TlsClient& operator=(TlsClient&&) noexcept;

    bool connect(const std::string& host, std::uint16_t port, const TlsConfig& config, std::string& error);
    bool add_root_certificate_pem(const std::string& pem, std::string& error);
    bool add_root_certificate_file(const std::string& path, std::string& error);
    std::size_t send(const std::string& data);
    std::string recv_some(std::size_t max_bytes = 4096);
    void close();
    bool is_open() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

}  // namespace requests_cpp::tls
