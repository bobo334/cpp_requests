#pragma once

#include <memory>
#include <string>

#include "requests_cpp/adapter.hpp"
#include "requests_cpp/connection_pool.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/url.hpp"

namespace requests_cpp::http1 {

std::shared_ptr<Adapter> create_http1_adapter(ConnectionPoolConfig config);

std::string serialize_request(const Request& request, const Url& url);
Response parse_response(const std::string& raw);
std::size_t find_complete_body_end(const std::string& raw, std::size_t header_end, const Headers& headers);
std::size_t find_complete_body_end(const std::string& raw,
                                   std::size_t header_end,
                                   const Headers& headers,
                                   int status_code,
                                   const std::string& method);

}  // namespace requests_cpp::http1
