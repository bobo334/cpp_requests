#include "requests_cpp/request.hpp"

namespace requests_cpp {

Request::Request(std::string url) : url_(std::move(url)) {}

const std::string& Request::url() const noexcept {
    return url_;
}

void Request::set_url(std::string url) {
    url_ = std::move(url);
}

const Headers& Request::headers() const noexcept {
    return headers_;
}

void Request::set_headers(Headers headers) {
    headers_ = std::move(headers);
}

const std::string& Request::method() const noexcept {
    return method_;
}

void Request::set_method(std::string method) {
    method_ = std::move(method);
}

const std::string& Request::body() const noexcept {
    return body_;
}

void Request::set_body(std::string body) {
    body_ = std::move(body);
}

const Timeout& Request::timeout() const noexcept {
    return timeout_;
}

void Request::set_timeout(Timeout timeout) {
    timeout_ = timeout;
}

int Request::retries() const noexcept {
    return retries_;
}

void Request::set_retries(int retries) {
    retries_ = retries;
}

}  // namespace requests_cpp
