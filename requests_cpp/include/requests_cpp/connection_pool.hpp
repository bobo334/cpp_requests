#pragma once

#include <cstddef>

namespace requests_cpp::http1 {

struct ConnectionPoolConfig {
    std::size_t max_connections{64};
    std::size_t max_connections_per_host{8};
    int idle_timeout_ms{30000};
    int health_check_interval_ms{5000};
};

}  // namespace requests_cpp::http1
