#pragma once

#include <memory>
#include <string>

#include "requests_cpp/adapter.hpp"
#include "requests_cpp/connection_pool.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/export.hpp"
#include "requests_cpp/proxy.hpp"
#include "requests_cpp/auth.hpp"
#include "requests_cpp/cookie.hpp"
#include "requests_cpp/retry.hpp"

namespace requests_cpp {

class REQUESTS_CPP_API Session {
public:
    Session();

    Response request(const std::string& method, const std::string& url);
    Response request(const std::string& method, const std::string& url, const std::string& body);
    Response request(const std::string& method, const std::string& url, const std::string& body, const Timeout& timeout,
                     int retries = 0);
    Response get(const std::string& url);
    Response post(const std::string& url);
    Response post(const std::string& url, const std::string& body);

    void mount(std::shared_ptr<Adapter> adapter);
    void set_connection_pool_config(http1::ConnectionPoolConfig config);
    void enable_http2(bool enable);
    
    // Proxy methods
    void set_proxy_config(const ProxyConfig& config);
    void set_proxy_from_environment();
    
    // Authentication methods
    void set_auth(std::shared_ptr<Auth> auth);
    void set_basic_auth(const std::string& username, const std::string& password);
    void set_bearer_token_auth(const std::string& token);
    void set_digest_auth(const std::string& username, const std::string& password);
    void set_ntlm_auth(const std::string& username, const std::string& password, 
                       const std::string& domain = "", const std::string& workstation = "");
    
    // Cookie methods
    void set_cookies(std::shared_ptr<CookieJar> cookie_jar);
    CookieJar& cookies();
    
    // Retry methods
    void set_retry_config(const RetryConfig& config);
    void set_max_retries(int retries);

private:
    std::shared_ptr<Adapter> adapter_{};
    Headers default_headers_{};
    Timeout default_timeout_{};
    int max_redirects_{30};
    bool http2_enabled_{false};
    http1::ConnectionPoolConfig connection_pool_config_{};
    
    // New functionality
    ProxyManager proxy_manager_;
    std::shared_ptr<Auth> auth_;
    std::shared_ptr<CookieJar> cookie_jar_;
    RetryStrategy retry_strategy_;
};

}  // namespace requests_cpp
