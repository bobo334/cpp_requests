#include "requests_cpp/tls.hpp"

#include <utility>

namespace requests_cpp::tls {

struct TlsClient::Impl {
    bool open{false};
};

TlsClient::TlsClient() : impl_(std::make_unique<Impl>()) {}

TlsClient::~TlsClient() = default;

TlsClient::TlsClient(TlsClient&& other) noexcept : impl_(std::move(other.impl_)) {}

TlsClient& TlsClient::operator=(TlsClient&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

bool TlsClient::connect(const std::string&, std::uint16_t, const TlsConfig&, std::string& error) {
    error = "TLS backend not implemented";
    if (impl_) {
        impl_->open = false;
    }
    return false;
}

bool TlsClient::add_root_certificate_pem(const std::string&, std::string& error) {
    error = "TLS backend not implemented";
    return false;
}

bool TlsClient::add_root_certificate_file(const std::string&, std::string& error) {
    error = "TLS backend not implemented";
    return false;
}

std::size_t TlsClient::send(const std::string&) {
    return 0;
}

std::string TlsClient::recv_some(std::size_t) {
    return {};
}

void TlsClient::close() {
    if (impl_) {
        impl_->open = false;
    }
}

bool TlsClient::is_open() const noexcept {
    return impl_ && impl_->open;
}

}  // namespace requests_cpp::tls
