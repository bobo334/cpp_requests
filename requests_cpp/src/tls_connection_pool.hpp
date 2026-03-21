#pragma once

#include "requests_cpp/tls.hpp"
#include "requests_cpp/url.hpp"

#include <string>
#include <unordered_map>

namespace requests_cpp::tls {

class TlsConnectionPool {
public:
    TlsClient acquire(const Url& url, const TlsConfig& config, std::string& error);
    void release(const Url& url, TlsClient&& client, bool keep_alive);

private:
    std::unordered_map<std::string, TlsClient> pool_{};
};

}  // namespace requests_cpp::tls
