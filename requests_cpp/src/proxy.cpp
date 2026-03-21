#include "requests_cpp/proxy.hpp"
#include <cstdlib>
#include <algorithm>
#include <sstream>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

namespace requests_cpp {

ProxyConfig ProxyConfig::from_environment() {
    ProxyConfig config;
    
    // Get HTTP proxy from environment
    const char* http_proxy = std::getenv("HTTP_PROXY");
    if (!http_proxy) {
        http_proxy = std::getenv("http_proxy");
    }
    if (http_proxy) {
        config.http_proxy = std::string(http_proxy);
    }
    
    // Get HTTPS proxy from environment
    const char* https_proxy = std::getenv("HTTPS_PROXY");
    if (!https_proxy) {
        https_proxy = std::getenv("https_proxy");
    }
    if (https_proxy) {
        config.https_proxy = std::string(https_proxy);
    }
    
    // Get SOCKS proxy from environment
    const char* socks_proxy = std::getenv("SOCKS_PROXY");
    if (!socks_proxy) {
        socks_proxy = std::getenv("socks_proxy");
    }
    if (socks_proxy) {
        config.socks_proxy = std::string(socks_proxy);
    }
    
    // Get SOCKS4 proxy from environment
    const char* socks4_proxy = std::getenv("SOCKS4_PROXY");
    if (socks4_proxy) {
        config.socks4_proxy = std::string(socks4_proxy);
    }
    
    // Get SOCKS5 proxy from environment
    const char* socks5_proxy = std::getenv("SOCKS5_PROXY");
    if (socks5_proxy) {
        config.socks5_proxy = std::string(socks5_proxy);
    }
    
    // Get NO_PROXY hosts from environment
    const char* no_proxy = std::getenv("NO_PROXY");
    if (!no_proxy) {
        no_proxy = std::getenv("no_proxy");
    }
    if (no_proxy) {
        std::string no_proxy_str(no_proxy);
        std::stringstream ss(no_proxy_str);
        std::string item;
        
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(0, item.find_first_not_of(" \t"));
            item.erase(item.find_last_not_of(" \t") + 1);
            
            if (!item.empty()) {
                config.no_proxy_hosts.push_back(item);
            }
        }
    }
    
    return config;
}

ProxyManager::ProxyManager(const ProxyConfig& config) : config_(config) {}

std::string ProxyManager::get_proxy_for_url(const std::string& url) const {
    // Check if URL starts with https:// to determine which proxy to use
    if (url.substr(0, 8) == "https://") {
        if (!config_.https_proxy.empty()) {
            return config_.https_proxy;
        }
        // Fallback to http proxy if https proxy is not set
        return config_.http_proxy;
    } else if (url.substr(0, 7) == "http://") {
        if (!config_.http_proxy.empty()) {
            return config_.http_proxy;
        }
    }
    
    // Check for specific SOCKS proxies
    if (url.substr(0, 8) == "https://") {
        if (!config_.socks5_proxy.empty()) {
            return config_.socks5_proxy;
        } else if (!config_.socks4_proxy.empty()) {
            return config_.socks4_proxy;
        }
    }
    
    // Check for general SOCKS proxy as fallback
    if (!config_.socks_proxy.empty()) {
        return config_.socks_proxy;
    }
    
    return "";
}

bool ProxyManager::should_bypass_proxy(const std::string& host) const {
    return matches_no_proxy(host);
}

std::string ProxyManager::extract_host_from_url(const std::string& url) const {
    size_t start = 0;
    
    // Skip protocol (http:// or https://)
    if (url.substr(0, 7) == "http://") {
        start = 7;
    } else if (url.substr(0, 8) == "https://") {
        start = 8;
    }
    
    // Find the end of the host (before port or path)
    size_t end = url.find('/', start);
    if (end == std::string::npos) {
        end = url.find(':', start);  // Look for port
        if (end == std::string::npos) {
            end = url.length();  // No port, take the whole thing
        }
    }
    
    return url.substr(start, end - start);
}

bool ProxyManager::matches_no_proxy(const std::string& host) const {
    for (const auto& pattern : config_.no_proxy_hosts) {
        // Check for exact match
        if (host == pattern) {
            return true;
        }
        
        // Check for wildcard match (pattern starting with .)
        if (pattern[0] == '.' && host.length() >= pattern.length()) {
            std::string suffix = host.substr(host.length() - pattern.length() + 1);
            if (suffix == pattern.substr(1)) {
                return true;
            }
        }
        
        // Check for IP address range match (CIDR notation)
        if (pattern.find('/') != std::string::npos) {
            // Simple CIDR matching (would need more sophisticated implementation in real code)
            if (host == pattern.substr(0, pattern.find('/'))) {
                return true;
            }
        }
    }
    
    return false;
}

bool ProxyManager::requires_connect_tunnel(const std::string& proxy_url, const std::string& dest_url) const {
    // Check if destination URL is HTTPS
    bool is_https = dest_url.substr(0, 8) == "https://";
    
    // Check proxy type
    std::string proxy_type = get_proxy_type(proxy_url);
    
    // CONNECT tunnel is required for HTTPS destinations through HTTP proxies
    if (is_https && (proxy_type == "http" || proxy_type == "https")) {
        return true;
    }
    
    return false;
}

std::string ProxyManager::get_proxy_type(const std::string& proxy_url) const {
    if (proxy_url.substr(0, 7) == "socks5:") {
        return "socks5";
    } else if (proxy_url.substr(0, 6) == "socks4:") {
        return "socks4";
    } else if (proxy_url.substr(0, 5) == "socks") {
        return "socks";
    } else if (proxy_url.substr(0, 6) == "https:") {
        return "https";
    } else if (proxy_url.substr(0, 5) == "http:") {
        return "http";
    }
    
    return "unknown";
}

}  // namespace requests_cpp