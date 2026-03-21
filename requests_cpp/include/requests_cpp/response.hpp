#pragma once

#include <string>

#include "requests_cpp/exceptions.hpp"
#include "requests_cpp/export.hpp"
#include "requests_cpp/types.hpp"

namespace requests_cpp {

class REQUESTS_CPP_API Response {
public:
    Response() = default;

    int status_code() const noexcept;
    void set_status_code(int status_code);

    const std::string& text() const noexcept;
    void set_text(std::string text);

    const Headers& headers() const noexcept;
    void set_headers(Headers headers);

    const std::string& raw() const noexcept;
    void set_raw(std::string raw);

    ErrorCode error_code() const noexcept;
    void set_error_code(ErrorCode code);

    int system_error_code() const noexcept;
    void set_system_error_code(int system_code);

    const std::string& error_url() const noexcept;
    void set_error_url(std::string url);

private:
    int status_code_{0};
    std::string text_{};
    Headers headers_{};
    std::string raw_{};
    ErrorCode error_code_{ErrorCode::Unknown};
    int system_error_code_{0};
    std::string error_url_{};
};

}  // namespace requests_cpp
