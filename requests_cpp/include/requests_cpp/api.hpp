#pragma once

#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/session.hpp"

namespace requests_cpp {

Response request(const std::string& method, const std::string& url);
Response request(const std::string& method, const std::string& url, const std::string& body);
Response request(const std::string& method, const std::string& url, const std::string& body, const Timeout& timeout,
                 int retries = 0);
Response get(const std::string& url);
Response post(const std::string& url);
Response post(const std::string& url, const std::string& body);

}  // namespace requests_cpp
