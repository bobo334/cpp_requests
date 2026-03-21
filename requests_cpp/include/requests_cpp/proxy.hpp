#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace requests_cpp {

struct ProxyConfig {
    std::string http_proxy;
    std::string https_proxy;
    std::string socks_proxy;
    std::string socks4_proxy;
    std::string socks5_proxy;
    std::vector<std::string> no_proxy_hosts;
    
    // Parse proxy settings from environment variables
    static ProxyConfig from_environment();
};

class ProxyManager {
public:
    ProxyManager() = default;
    explicit ProxyManager(const ProxyConfig& config);
    
    // Get proxy URL for a given destination URL
    std::string get_proxy_for_url(const std::string& url) const;
    
    // Check if a host should bypass proxy
    bool should_bypass_proxy(const std::string& host) const;
    
    // Check if proxy requires CONNECT tunnel
    bool requires_connect_tunnel(const std::string& proxy_url, const std::string& dest_url) const;
    
    // Get proxy type (http, https, socks4, socks5)
    std::string get_proxy_type(const std::string& proxy_url) const;
    
private:
    ProxyConfig config_;
    
    // Helper to parse host from URL
    std::string extract_host_from_url(const std::string& url) const;
    
    // Helper to check if host matches no_proxy patterns
    bool matches_no_proxy(const std::string& host) const;
};

}  // namespace requests_cpp