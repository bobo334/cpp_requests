#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace requests_cpp {

struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::chrono::system_clock::time_point expires;
    bool secure;
    bool httponly;
    std::string samesite;
    
    Cookie() : expires(std::chrono::system_clock::time_point::max()), secure(false), httponly(false) {}
    
    // Check if cookie is expired
    bool is_expired() const;
    
    // Check if cookie matches a given domain and path
    bool matches_domain_path(const std::string& domain, const std::string& path) const;
};

class CookieJar {
public:
    CookieJar() = default;
    
    // Add a cookie to the jar
    void add_cookie(const Cookie& cookie);
    
    // Get cookies for a specific domain and path
    std::vector<Cookie> get_cookies(const std::string& domain, const std::string& path) const;
    
    // Get cookie header string for a specific domain and path
    std::string get_cookie_header(const std::string& domain, const std::string& path) const;
    
    // Parse Set-Cookie header and add to jar
    void parse_set_cookie_header(const std::string& set_cookie_header, const std::string& origin_domain = "");
    
    // Clear expired cookies
    void clear_expired();
    
    // Clear all cookies
    void clear();
    
    // Persistence methods
    bool save_to_file(const std::string& filepath) const;
    bool load_from_file(const std::string& filepath);
    bool save_as_mozilla_format(const std::string& filepath) const;
    bool load_from_mozilla_format(const std::string& filepath);
    
private:
    std::vector<Cookie> cookies_;
    
    // Helper to normalize domain (add leading dot if not present)
    std::string normalize_domain(const std::string& domain) const;
    
    // Helper to check if domain matches
    bool domain_matches(const std::string& cookie_domain, const std::string& request_domain) const;
};

}  // namespace requests_cpp