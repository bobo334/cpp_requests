#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "requests_cpp/adapter.hpp"
#include "requests_cpp/connection_pool.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/url.hpp"

namespace requests_cpp::http2 {

struct Http2Config {
    std::size_t max_connections{64};
    std::size_t max_connections_per_host{8};
    std::uint32_t max_concurrent_streams{128};
    std::uint32_t initial_stream_window_size{65535};
    std::uint32_t initial_connection_window_size{65535};
    std::uint32_t header_table_size{4096};
    int idle_timeout_ms{30000};
};

struct Http2AdapterResult {
    std::shared_ptr<Adapter> adapter;
    bool fallback_to_http1{true};
    std::string error;
};

class Http2Adapter : public Adapter {
public:
    explicit Http2Adapter(Http2Config config);
    ~Http2Adapter();

    Response send(const Request& request) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

Http2AdapterResult create_http2_adapter(Http2Config config);
Http2AdapterResult create_http2_adapter();

}  // namespace requests_cpp::http2
