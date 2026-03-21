#include "requests_cpp/exceptions.hpp"

#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#endif

namespace requests_cpp {

namespace {
class RequestsCppErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return "requests_cpp";
    }

    std::string message(int) const override {
        return "requests_cpp error";
    }
};

const RequestsCppErrorCategory kCategory{};

const char* error_code_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::Network:
            return "Network";
        case ErrorCode::Timeout:
            return "Timeout";
        case ErrorCode::TLS:
            return "TLS";
        case ErrorCode::HTTP2:
            return "HTTP2";
        case ErrorCode::IO:
            return "IO";
        case ErrorCode::Protocol:
            return "Protocol";
        case ErrorCode::Unknown:
        default:
            return "Unknown";
    }
}

std::string format_context_suffix(const ErrorContext& context) {
    std::string details;
    auto append = [&details](const std::string& key, const std::string& value) {
        if (value.empty()) {
            return;
        }
        if (!details.empty()) {
            details += ", ";
        }
        details += key + "=" + value;
    };

    append("url", context.request_url);
    append("method", context.method);
    if (context.status_code != 0) {
        append("status", std::to_string(context.status_code));
    }
    append("tls", context.tls_info);
    if (context.system_code != 0) {
        append("sys", std::to_string(context.system_code));
    }
    if (context.tls_error != TlsError::None) {
        append("tls_error", tls_error_name(context.tls_error));
    }

    if (details.empty()) {
        return "";
    }
    return " [" + details + "]";
}
}  // namespace

const std::error_category& requests_cpp_error_category() {
    return kCategory;
}

const char* tls_error_name(TlsError error) {
    switch (error) {
        case TlsError::None:
            return "None";
        case TlsError::HostnameMismatch:
            return "HostnameMismatch";
        case TlsError::CertExpired:
            return "CertExpired";
        case TlsError::ChainUntrusted:
            return "ChainUntrusted";
        case TlsError::PinningFailed:
            return "PinningFailed";
        case TlsError::Unknown:
        default:
            return "Unknown";
    }
}

std::string format_system_message(int system_code) {
    if (system_code == 0) {
        return "";
    }
#if defined(_WIN32)
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD size = FormatMessageA(flags, nullptr, static_cast<DWORD>(system_code), 0,
                                      reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    if (size == 0 || buffer == nullptr) {
        return "";
    }
    std::string message(buffer, size);
    LocalFree(buffer);
    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    return message;
#else
    return std::string(std::strerror(system_code));
#endif
}

std::string format_error_message(ErrorCode code, const std::string& message, int system_code) {
    std::string result = std::string("[") + error_code_name(code) + "] " + message;
    const std::string sys_message = format_system_message(system_code);
    if (!sys_message.empty()) {
        result += " (" + std::to_string(system_code) + ": " + sys_message + ")";
    } else if (system_code != 0) {
        result += " (" + std::to_string(system_code) + ")";
    }
    return result;
}

RequestException::RequestException(ErrorCode code, const std::string& message, int system_code)
    : std::runtime_error(format_error_message(code, message, system_code)),
      code_(code),
      system_code_(system_code) {
    context_.system_code = system_code;
}

RequestException::RequestException(ErrorCode code, const std::string& message, ErrorContext context)
    : std::runtime_error(format_error_message(code, message, context.system_code) + format_context_suffix(context)),
      code_(code),
      system_code_(context.system_code),
      context_(std::move(context)) {}

ErrorCode RequestException::code() const noexcept {
    return code_;
}

int RequestException::system_code() const noexcept {
    return system_code_;
}

std::error_code RequestException::error_code() const noexcept {
    return {static_cast<int>(code_), requests_cpp_error_category()};
}

const ErrorContext& RequestException::context() const noexcept {
    return context_;
}

}  // namespace requests_cpp
