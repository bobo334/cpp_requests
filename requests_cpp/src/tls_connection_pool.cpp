#include "tls_connection_pool.hpp"

namespace requests_cpp::tls {

namespace {
std::string make_key(const Url& url) {
    const std::uint16_t port = url.has_port ? url.port : 443;
    return url.scheme + "://" + url.host + ":" + std::to_string(port);
}
}  // namespace

TlsClient TlsConnectionPool::acquire(const Url& url, const TlsConfig& config, std::string& error) {
    const std::string key = make_key(url);
    auto it = pool_.find(key);
    if (it != pool_.end()) {
        TlsClient client = std::move(it->second);
        pool_.erase(it);
        if (client.is_open()) {
            return client;
        }
    }

    TlsClient client;
    const std::uint16_t port = url.has_port ? url.port : 443;
    if (!client.connect(url.host, port, config, error)) {
        return {};
    }
    return client;
}

void TlsConnectionPool::release(const Url& url, TlsClient&& client, bool keep_alive) {
    if (!keep_alive || !client.is_open()) {
        client.close();
        return;
    }

    pool_[make_key(url)] = std::move(client);
}

}  // namespace requests_cpp::tls
