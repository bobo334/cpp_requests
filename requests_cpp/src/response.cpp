#include "requests_cpp/response.hpp"

namespace requests_cpp {

int Response::status_code() const noexcept {
    return status_code_;
}

void Response::set_status_code(int status_code) {
    status_code_ = status_code;
}

const std::string& Response::text() const noexcept {
    return text_;
}

void Response::set_text(std::string text) {
    text_ = std::move(text);
}

const Headers& Response::headers() const noexcept {
    return headers_;
}

void Response::set_headers(Headers headers) {
    headers_ = std::move(headers);
}

const std::string& Response::raw() const noexcept {
    return raw_;
}

void Response::set_raw(std::string raw) {
    raw_ = std::move(raw);
}

ErrorCode Response::error_code() const noexcept {
    return error_code_;
}

void Response::set_error_code(ErrorCode code) {
    error_code_ = code;
}

int Response::system_error_code() const noexcept {
    return system_error_code_;
}

void Response::set_system_error_code(int system_code) {
    system_error_code_ = system_code;
}

const std::string& Response::error_url() const noexcept {
    return error_url_;
}

void Response::set_error_url(std::string url) {
    error_url_ = std::move(url);
}

}  // namespace requests_cpp
