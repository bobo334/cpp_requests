#include "requests_cpp/tls.hpp"

#include "cert/cert.h"
#include "net/TcpSock.h"
#include "ssl/TinyTls.h"
#include "ssl/TlsFactory.h"
#include "ssl/cipher.h"
#include "ssl/ssl_defs.h"

#include "root_certs.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace requests_cpp::tls {

namespace {

extern "C" SSL_RESULT SSL_AddRootCertificate(const uchar* pCertData, uint nLen, uint nUnixTime);

bool read_file_bytes(const std::string& path, std::vector<unsigned char>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.good();
}

bool is_pem_data(const std::vector<unsigned char>& data) {
    static const std::string kPemBegin = "-----BEGIN CERTIFICATE-----";
    if (data.size() < kPemBegin.size()) {
        return false;
    }
    const std::string view(reinterpret_cast<const char*>(data.data()), data.size());
    return view.find(kPemBegin) != std::string::npos;
}

bool add_root_certificate_der(const std::vector<unsigned char>& der, std::string& error) {
    SSL_RESULT result = SSL_AddRootCertificate(der.data(), static_cast<uint>(der.size()), 0);
    if (result != SSL_OK) {
        error = "TinyTls add root certificate failed";
        return false;
    }
    return true;
}

int base64_value(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

bool decode_base64(const std::string& input, std::vector<unsigned char>& output) {
    output.clear();
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (c == '=' || c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            continue;
        }
        int d = base64_value(static_cast<char>(c));
        if (d < 0) {
            return false;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return !output.empty();
}

bool load_root_cert_pem_bundle(const std::string& pem, std::string& error) {
    static const std::string kBegin = "-----BEGIN CERTIFICATE-----";
    static const std::string kEnd = "-----END CERTIFICATE-----";
    std::size_t pos = 0;
    bool loaded = false;

    while (true) {
        std::size_t begin = pem.find(kBegin, pos);
        if (begin == std::string::npos) {
            break;
        }
        begin += kBegin.size();
        std::size_t end = pem.find(kEnd, begin);
        if (end == std::string::npos) {
            break;
        }
        std::string b64 = pem.substr(begin, end - begin);
        std::vector<unsigned char> der;
        if (!decode_base64(b64, der)) {
            error = "TinyTls PEM base64 decode failed";
            return false;
        }
        if (!add_root_certificate_der(der, error)) {
            return false;
        }
        loaded = true;
        pos = end + kEnd.size();
    }

    if (!loaded) {
        error = "TinyTls PEM bundle contains no certificates";
        return false;
    }
    return true;
}

bool load_root_cert_from_file(const std::string& path, std::string& error) {
    std::vector<unsigned char> content;
    if (!read_file_bytes(path, content)) {
        error = "TinyTls read CA file failed";
        return false;
    }
    if (is_pem_data(content)) {
        std::string pem(reinterpret_cast<const char*>(content.data()), content.size());
        return load_root_cert_pem_bundle(pem, error);
    }
    return add_root_certificate_der(content, error);
}

void load_root_certs(const CIPHERSET* cipher_set, const std::string& ca_path, bool verify, std::string& error) {
    if (!verify) {
        return;
    }
    StartCerts(nullptr, nullptr, cipher_set);
    if (!ca_path.empty()) {
        load_root_cert_from_file(ca_path, error);
        return;
    }
    if (kEmbeddedRootCertsSize > 0) {
        SSL_AddRootCertificate(reinterpret_cast<const uchar*>(kEmbeddedRootCertsPem),
                               static_cast<uint>(kEmbeddedRootCertsSize), 0);
    }
}

#if defined(_WIN32)
class WsaGuard {
public:
    WsaGuard() {
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
    }

    ~WsaGuard() { WSACleanup(); }
};
#endif

#if defined(_WIN32)
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
#endif

class TinyTlsClientSocket : public TcpSock {
public:
    TinyTlsClientSocket(const std::string& host, std::uint16_t port, const TlsConfig& config, std::string& error)
        : socket_(kInvalidSocket), valid_(false) {
        connect_socket(host, port, config, error);
    }

    ~TinyTlsClientSocket() override { close_socket(); }

    HSock GetSock() const override { return reinterpret_cast<HSock>(socket_); }

    bool Incoming() override {
        if (socket_ == kInvalidSocket) {
            return false;
        }

#if defined(_WIN32)
        fd_set readfds{};
        FD_ZERO(&readfds);
        FD_SET(socket_, &readfds);
        timeval timeout{0, 0};
        int result = select(0, &readfds, nullptr, nullptr, &timeout);
        return result > 0 && FD_ISSET(socket_, &readfds);
#else
        fd_set readfds{};
        FD_ZERO(&readfds);
        FD_SET(socket_, &readfds);
        timeval timeout{0, 0};
        int result = select(socket_ + 1, &readfds, nullptr, nullptr, &timeout);
        return result > 0 && FD_ISSET(socket_, &readfds);
#endif
    }

    HSock Accept(const TcpSock&) override { return nullptr; }

    bool Connected() override { return valid_ && socket_ != kInvalidSocket; }

    bool Closed() override { return socket_ == kInvalidSocket; }

    int Send(const uint8_t* pData, size_t cbSize) override {
        if (!Connected()) {
            return 0;
        }
#if defined(_WIN32)
        int sent = ::send(socket_, reinterpret_cast<const char*>(pData), static_cast<int>(cbSize), 0);
#else
        int sent = static_cast<int>(::send(socket_, pData, cbSize, 0));
#endif
        return sent > 0 ? sent : 0;
    }

    int Recv(uint8_t* pData, size_t cbSize) override {
        if (!Connected()) {
            return 0;
        }
#if defined(_WIN32)
        int received = ::recv(socket_, reinterpret_cast<char*>(pData), static_cast<int>(cbSize), 0);
#else
        int received = static_cast<int>(::recv(socket_, pData, cbSize, 0));
#endif
        return received > 0 ? received : 0;
    }

    bool is_valid() const noexcept { return valid_; }

private:
    void connect_socket(const std::string& host, std::uint16_t port, const TlsConfig& config, std::string& error) {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        const std::string port_str = std::to_string(port);
        if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0 || !result) {
            error = "TinyTls getaddrinfo failed";
            return;
        }

        for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
#if defined(_WIN32)
            SocketHandle sock = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
#else
            SocketHandle sock = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
#endif
            if (sock == kInvalidSocket) {
                continue;
            }

            apply_timeouts(sock, config);

            if (::connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
                socket_ = sock;
                valid_ = true;
                break;
            }

#if defined(_WIN32)
            ::closesocket(sock);
#else
            ::close(sock);
#endif
        }

        freeaddrinfo(result);

        if (!valid_) {
            error = "TinyTls socket connect failed";
        }
    }

    void apply_timeouts(SocketHandle sock, const TlsConfig& config) {
        const auto connect_ms = config.connect_timeout_ms;
        const auto read_ms = config.read_timeout_ms;
        const auto write_ms = config.write_timeout_ms;

        const auto socket_timeout = static_cast<long>(connect_ms > 0 ? connect_ms
                                            : (read_ms > 0 ? read_ms : write_ms));
        if (socket_timeout <= 0) {
            return;
        }

#if defined(_WIN32)
        DWORD timeout = static_cast<DWORD>(socket_timeout);
        ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
        timeval tv{};
        tv.tv_sec = socket_timeout / 1000;
        tv.tv_usec = (socket_timeout % 1000) * 1000;
        ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
    }

    void close_socket() {
        if (socket_ == kInvalidSocket) {
            return;
        }
#if defined(_WIN32)
        ::closesocket(socket_);
#else
        ::close(socket_);
#endif
        socket_ = kInvalidSocket;
        valid_ = false;
    }

    SocketHandle socket_;
    bool valid_;
};

struct CallbackContext {
    bool verify{true};
    std::string server_name;
    std::string last_error;
};

unsigned int tls_callback(void* context, TlsCBData* cb) {
    auto* ctx = static_cast<CallbackContext*>(context);
    switch (cb->cbType) {
        case TlsCBData::CB_CLIENT_CIPHER: {
            const TLS_CIPHER ciphers[] = {
                TLS_AES_128_GCM_SHA256,
                TLS_CHACHA20_POLY1305_SHA256,
                TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
                TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
                TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
                TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
                TLS_NONE
            };
            unsigned int i = 0;
            for (; ciphers[i] != TLS_NONE; ++i) {
                cb->data.rawInt[i] = ciphers[i];
            }
            cb->data.rawInt[i] = 0;
            return i;
        }
        case TlsCBData::CB_SERVER_NAME: {
            if (ctx && !ctx->server_name.empty()) {
                cb->data.ptrs[0] = const_cast<char*>(ctx->server_name.c_str());
                cb->data.rawSize[1] = ctx->server_name.size();
                return static_cast<unsigned int>(cb->data.rawSize[1]);
            }
            return static_cast<unsigned int>(cb->data.rawSize[1]);
        }
        case TlsCBData::CB_CERTIFICATE_ALERT: {
            if (ctx && ctx->verify) {
                ctx->last_error = "TinyTls certificate verification failed";
                return 0;
            }
            return 1;
        }
        default:
            return 0;
    }
}

}  // namespace

struct TlsClient::Impl {
#if defined(_WIN32)
    WsaGuard guard{};
#endif
    std::unique_ptr<TinyTlsClientSocket> socket;
    std::unique_ptr<BaseTls> tls;
    std::unique_ptr<CallbackContext> callback;
    bool open{false};
};

TlsClient::TlsClient() : impl_(std::make_unique<Impl>()) {}

TlsClient::~TlsClient() { close(); }

TlsClient::TlsClient(TlsClient&& other) noexcept : impl_(std::move(other.impl_)) {}

TlsClient& TlsClient::operator=(TlsClient&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

bool TlsClient::connect(const std::string& host, std::uint16_t port, const TlsConfig& config, std::string& error) {
    if (!impl_) {
        error = "TLS client not initialized";
        return false;
    }

    auto cipherSet = gCipherSet;
    InitCiphers(&cipherSet);
    load_root_certs(&cipherSet, config.ca_path, config.verify, error);
    if (!error.empty()) {
        return false;
    }

    impl_->callback = std::make_unique<CallbackContext>();
    impl_->callback->verify = config.verify;
    impl_->callback->server_name = host;

    impl_->socket = std::make_unique<TinyTlsClientSocket>(host, port, config, error);
    if (!impl_->socket->is_valid()) {
        return false;
    }

    impl_->tls.reset(CreateTls(*impl_->socket, cipherSet, static_cast<unsigned int>(std::time(nullptr)), true,
                               tls_callback, impl_->callback.get()));
    if (!impl_->tls) {
        error = "TinyTls CreateTls failed";
        return false;
    }

    SSL_STATE state = SSLSTATE_TCPCONNECTED;
    for (int i = 0; i < 1000; ++i) {
        state = impl_->tls->Work(static_cast<unsigned int>(std::time(nullptr)), state);
        if (state == SSLSTATE_CONNECTED) {
            impl_->open = true;
            return true;
        }
        if (state == SSLSTATE_ABORT || state == SSLSTATE_ABORTED || state == SSLSTATE_ERROR) {
            if (impl_->callback && !impl_->callback->last_error.empty()) {
                error = impl_->callback->last_error;
            } else {
                error = "TinyTls handshake failed";
            }
            break;
        }
    }

    impl_->open = false;
    return false;
}

bool TlsClient::add_root_certificate_pem(const std::string& pem, std::string& error) {
    std::vector<unsigned char> content(pem.begin(), pem.end());
    if (content.empty()) {
        error = "TinyTls PEM input is empty";
        return false;
    }
    if (!is_pem_data(content)) {
        error = "TinyTls PEM input invalid";
        return false;
    }
    return load_root_cert_pem_bundle(pem, error);
}

bool TlsClient::add_root_certificate_file(const std::string& path, std::string& error) {
    return load_root_cert_from_file(path, error);
}

std::size_t TlsClient::send(const std::string& data) {
    if (!impl_ || !impl_->open || !impl_->tls) {
        return 0;
    }
    int written = impl_->tls->Write(reinterpret_cast<const unsigned char*>(data.data()), data.size());
    return written > 0 ? static_cast<std::size_t>(written) : 0;
}

std::string TlsClient::recv_some(std::size_t max_bytes) {
    if (!impl_ || !impl_->open || !impl_->tls) {
        return {};
    }
    std::string buffer(max_bytes, '\0');
    int read = impl_->tls->Read(reinterpret_cast<unsigned char*>(buffer.data()), max_bytes);
    if (read <= 0) {
        return {};
    }
    buffer.resize(static_cast<std::size_t>(read));
    return buffer;
}

void TlsClient::close() {
    if (!impl_) {
        return;
    }
    impl_->tls.reset();
    impl_->socket.reset();
    impl_->callback.reset();
    impl_->open = false;
}

bool TlsClient::is_open() const noexcept { return impl_ && impl_->open; }

}  // namespace requests_cpp::tls
