#pragma once

#include <memory>
#include <string>

#include "requests_cpp/export.hpp"
#include "requests_cpp/types.hpp"

namespace requests_cpp {

class REQUESTS_CPP_API Request {
public:
    Request() = default;
    explicit Request(std::string url);

    const std::string& url() const noexcept;
    void set_url(std::string url);

    const Headers& headers() const noexcept;
    void set_headers(Headers headers);

    const std::string& method() const noexcept;
    void set_method(std::string method);

    const std::string& body() const noexcept;
    void set_body(std::string body);

    const Timeout& timeout() const noexcept;
    void set_timeout(Timeout timeout);

    int retries() const noexcept;
    void set_retries(int retries);

private:
    std::string method_{"GET"};
    std::string url_;
    Headers headers_{};
    std::string body_{};
    Timeout timeout_{};
    int retries_{0};
};

}  // namespace requests_cpp
