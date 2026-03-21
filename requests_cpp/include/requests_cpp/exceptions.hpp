#pragma once

#include <string>
#include <system_error>

#include "requests_cpp/export.hpp"

namespace requests_cpp {

enum class ErrorCode {
    Network,
    Timeout,
    TLS,
    HTTP2,
    IO,
    Protocol,
    Unknown
};

enum class TlsError {
    None,
    HostnameMismatch,
    CertExpired,
    ChainUntrusted,
    PinningFailed,
    Unknown
};

struct ErrorContext {
    std::string request_url{};
    std::string method{};
    int status_code{0};
    std::string tls_info{};
    int system_code{0};
    TlsError tls_error{TlsError::None};
};

const std::error_category& requests_cpp_error_category();
const char* tls_error_name(TlsError error);

std::string format_system_message(int system_code);
std::string format_error_message(ErrorCode code, const std::string& message, int system_code);

class RequestException : public std::runtime_error {
public:
    RequestException(ErrorCode code, const std::string& message, int system_code = 0);
    RequestException(ErrorCode code, const std::string& message, ErrorContext context);

    ErrorCode code() const noexcept;
    int system_code() const noexcept;
    std::error_code error_code() const noexcept;
    const ErrorContext& context() const noexcept;

private:
    ErrorCode code_{};
    int system_code_{};
    ErrorContext context_{};
};

class ConnectionError : public RequestException {
public:
    using RequestException::RequestException;
};

class TimeoutError : public RequestException {
public:
    using RequestException::RequestException;
};

class SSLError : public RequestException {
public:
    using RequestException::RequestException;
};

class IOError : public RequestException {
public:
    using RequestException::RequestException;
};

class ProtocolError : public RequestException {
public:
    using RequestException::RequestException;
};

class HTTP2Error : public RequestException {
public:
    using RequestException::RequestException;
};

class InvalidSchemaError : public RequestException {
public:
    using RequestException::RequestException;
};

class InvalidURLError : public RequestException {
public:
    using RequestException::RequestException;
};

class HTTPError : public RequestException {
public:
    using RequestException::RequestException;
};

}  // namespace requests_cpp
