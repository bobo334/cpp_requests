#include "requests_cpp/session.hpp"

#include "requests_cpp/exceptions.hpp"
#include "requests_cpp/http2.hpp"
#include "requests_cpp/url.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <utility>

namespace requests_cpp {

std::shared_ptr<Adapter> create_http1_adapter();
std::shared_ptr<Adapter> create_http1_adapter(http1::ConnectionPoolConfig config);

namespace {
Headers merge_headers(const Headers& defaults, const Headers& overrides) {
    Headers result = defaults;
    for (const auto& [key, value] : overrides) {
        result[key] = value;
    }
    return result;
}

Timeout merge_timeout(const Timeout& defaults, const Timeout& overrides) {
    Timeout result = overrides;
    if (result.connect_ms <= 0) {
        result.connect_ms = defaults.connect_ms;
    }
    if (result.read_ms <= 0) {
        result.read_ms = defaults.read_ms;
    }
    return result;
}

bool is_redirect_status(int status) {
    return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

bool should_switch_to_get(int status, const std::string& method) {
    if (status == 303) {
        return true;
    }
    if (status == 301 || status == 302) {
        std::string lower = method;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return lower != "get" && lower != "head";
    }
    return false;
}

void strip_body_headers(Headers& headers) {
    headers.erase("Content-Length");
    headers.erase("Content-Type");
    headers.erase("Transfer-Encoding");
}

std::string resolve_redirect(const std::string& base_url, const std::string& location) {
    if (location.rfind("http://", 0) == 0 || location.rfind("https://", 0) == 0) {
        return location;
    }

    Url base = Url::parse(base_url);
    if (location.rfind("//", 0) == 0) {
        return base.scheme + ":" + location;
    }
    if (!location.empty() && location.front() == '/') {
        return base.scheme + "://" + base.authority() + location;
    }

    std::string base_path = base.path.empty() ? "/" : base.path;
    auto slash = base_path.find_last_of('/');
    std::string prefix = slash == std::string::npos ? "/" : base_path.substr(0, slash + 1);
    return base.scheme + "://" + base.authority() + prefix + location;
}

Response send_with_retries(Adapter& adapter, Request request, int retries) {
    int attempts = std::max(0, retries) + 1;
    Response response;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        response = adapter.send(request);
        if (response.status_code() != 0) {
            return response;
        }
        if (attempt + 1 < attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    return response;
}
}  // namespace

Session::Session() : adapter_(create_http1_adapter()), cookie_jar_(std::make_shared<CookieJar>()) {
    default_headers_["User-Agent"] = "requests_cpp/0.1";
    default_headers_["Accept"] = "*/*";
    default_headers_["Accept-Encoding"] = "gzip, deflate, br";
}

void Session::set_connection_pool_config(http1::ConnectionPoolConfig config) {
    connection_pool_config_ = std::move(config);
    adapter_ = create_http1_adapter(connection_pool_config_);
}

void Session::enable_http2(bool enable) {
    http2_enabled_ = enable;
    if (http2_enabled_) {
        auto result = http2::create_http2_adapter();
        if (result.adapter && !result.fallback_to_http1) {
            adapter_ = std::move(result.adapter);
            return;
        }
    }
    adapter_ = create_http1_adapter(connection_pool_config_);
}

Response Session::request(const std::string& method, const std::string& url) {
    Timeout merged = merge_timeout(default_timeout_, {});
    return request(method, url, {}, merged, 0);
}

Response Session::request(const std::string& method, const std::string& url, const std::string& body) {
    Timeout merged = merge_timeout(default_timeout_, {});
    return request(method, url, body, merged, 0);
}

Response Session::request(const std::string& method, const std::string& url, const std::string& body,
                          const Timeout& timeout, int retries) {
    Request request(url);
    request.set_method(method);
    request.set_body(body);
    request.set_timeout(merge_timeout(default_timeout_, timeout));
    request.set_retries(retries);

    Headers headers = merge_headers(default_headers_, request.headers());

    // Add authentication header if configured
    if (auth_) {
        headers["Authorization"] = auth_->get_auth_header();
    }

    // Add cookies if configured
    if (cookie_jar_) {
        Url parsed_url = Url::parse(url);
        std::string cookie_header = cookie_jar_->get_cookie_header(parsed_url.host, parsed_url.path);
        if (!cookie_header.empty()) {
            headers["Cookie"] = cookie_header;
        }
    }

    Response response;
    std::string current_url = url;
    std::string current_method = method;
    std::string current_body = body;
    bool attempted_http2 = false;

    try {
        if (http2_enabled_) {
            auto result = http2::create_http2_adapter();
            if (result.adapter && !result.fallback_to_http1) {
                adapter_ = std::move(result.adapter);
                attempted_http2 = true;
            } else {
                adapter_ = create_http1_adapter(connection_pool_config_);
            }
        }
        for (int redirect = 0; redirect <= max_redirects_; ++redirect) {
            request.set_url(current_url);
            request.set_method(current_method);
            request.set_body(current_body);
            request.set_headers(headers);

            response = send_with_retries(*adapter_, request, retries);
            if (response.status_code() == 0 && attempted_http2 &&
                (response.error_code() == ErrorCode::HTTP2 || response.error_code() == ErrorCode::IO ||
                 response.error_code() == ErrorCode::TLS)) {
                adapter_ = create_http1_adapter(connection_pool_config_);
                attempted_http2 = false;
                response = send_with_retries(*adapter_, request, retries);
            }
            
            // Process cookies from response
            if (cookie_jar_) {
                Url parsed_url = Url::parse(current_url);
                for (const auto& [header_name, header_value] : response.headers()) {
                    if (header_name == "Set-Cookie" || header_name == "set-cookie") {
                        cookie_jar_->parse_set_cookie_header(header_value, parsed_url.host);
                    }
                }
            }
            
            if (!is_redirect_status(response.status_code())) {
                return response;
            }

            auto location_it = response.headers().find("Location");
            if (location_it == response.headers().end()) {
                return response;
            }

            current_url = resolve_redirect(current_url, location_it->second);
            if (should_switch_to_get(response.status_code(), current_method)) {
                current_method = "GET";
                current_body.clear();
                strip_body_headers(headers);
            }
        }

        return response;
    } catch (const RequestException& ex) {
        Response error_response;
        error_response.set_status_code(0);
        error_response.set_text(ex.what());
        if (!ex.context().request_url.empty()) {
            error_response.set_error_url(ex.context().request_url);
        }
        if (ex.code() != ErrorCode::Unknown) {
            error_response.set_error_code(ex.code());
        }
        if (ex.context().system_code != 0 || ex.system_code() != 0) {
            const int system_code = ex.context().system_code != 0 ? ex.context().system_code : ex.system_code();
            error_response.set_system_error_code(system_code);
        }
        return error_response;
    }
}

Response Session::get(const std::string& url) {
    return request("GET", url);
}

Response Session::post(const std::string& url) {
    return request("POST", url);
}

Response Session::post(const std::string& url, const std::string& body) {
    return request("POST", url, body);
}

void Session::mount(std::shared_ptr<Adapter> adapter) {
    if (adapter) {
        adapter_ = std::move(adapter);
    }
}

// Proxy methods
void Session::set_proxy_config(const ProxyConfig& config) {
    proxy_manager_ = ProxyManager(config);
}

void Session::set_proxy_from_environment() {
    ProxyConfig config = ProxyConfig::from_environment();
    proxy_manager_ = ProxyManager(config);
}

// Authentication methods
void Session::set_auth(std::shared_ptr<Auth> auth) {
    auth_ = auth;
}

void Session::set_basic_auth(const std::string& username, const std::string& password) {
    auth_ = std::make_shared<BasicAuth>(username, password);
}

void Session::set_bearer_token_auth(const std::string& token) {
    auth_ = std::make_shared<BearerTokenAuth>(token);
}

void Session::set_digest_auth(const std::string& username, const std::string& password) {
    auth_ = std::make_shared<DigestAuth>(username, password);
}

void Session::set_ntlm_auth(const std::string& username, const std::string& password, 
                            const std::string& domain, const std::string& workstation) {
    auth_ = std::make_shared<NtlmAuth>(username, password, domain, workstation);
}

// Cookie methods
void Session::set_cookies(std::shared_ptr<CookieJar> cookie_jar) {
    cookie_jar_ = cookie_jar;
}

CookieJar& Session::cookies() {
    return *cookie_jar_;
}

// Retry methods
void Session::set_retry_config(const RetryConfig& config) {
    retry_strategy_ = RetryStrategy(config);
}

void Session::set_max_retries(int retries) {
    RetryConfig config;
    config.total_retries = retries;
    retry_strategy_ = RetryStrategy(config);
}

}  // namespace requests_cpp
