#include "requests_cpp/cookie.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>

namespace requests_cpp {

bool Cookie::is_expired() const {
    auto now = std::chrono::system_clock::now();
    return expires < now;
}

bool Cookie::matches_domain_path(const std::string& request_domain, const std::string& request_path) const {
    // 使用最小可用规则：域名完全相同或请求域名以 Cookie 域名结尾。
    const std::string& cookie_domain = domain;
    if (!cookie_domain.empty()) {
        const bool is_exact_match = request_domain == cookie_domain;
        const bool is_suffix_match =
            request_domain.size() > cookie_domain.size() &&
            request_domain.substr(request_domain.size() - cookie_domain.size()) == cookie_domain;
        if (!is_exact_match && !is_suffix_match) {
            return false;
        }
    }

    // 请求路径需要以 Cookie 路径为前缀。
    const std::string& cookie_path = this->path;
    if (!cookie_path.empty()) {
        if (request_path.size() < cookie_path.size()) {
            return false;
        }
        if (request_path.substr(0, cookie_path.size()) != cookie_path) {
            return false;
        }
    }

    return true;
}

void CookieJar::add_cookie(const Cookie& cookie) {
    // Remove existing cookie with same name, domain, and path
    for (auto it = cookies_.begin(); it != cookies_.end();) {
        if (it->name == cookie.name && 
            normalize_domain(it->domain) == normalize_domain(cookie.domain) &&
            it->path == cookie.path) {
            it = cookies_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Add the new cookie
    cookies_.push_back(cookie);
}

std::vector<Cookie> CookieJar::get_cookies(const std::string& domain, const std::string& path) const {
    std::vector<Cookie> result;
    
    for (const auto& cookie : cookies_) {
        if (!cookie.is_expired() && 
            cookie.matches_domain_path(domain, path)) {
            result.push_back(cookie);
        }
    }
    
    return result;
}

std::string CookieJar::get_cookie_header(const std::string& domain, const std::string& path) const {
    auto cookies = get_cookies(domain, path);
    
    if (cookies.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& cookie : cookies) {
        if (!first) {
            oss << "; ";
        }
        oss << cookie.name << "=" << cookie.value;
        first = false;
    }
    
    return oss.str();
}

void CookieJar::parse_set_cookie_header(const std::string& set_cookie_header, const std::string& origin_domain) {
    if (set_cookie_header.empty()) {
        return;
    }
    
    Cookie cookie;
    
    // Parse the Set-Cookie header
    std::string header = set_cookie_header;
    size_t pos = 0;
    
    // Parse the name=value part first
    size_t eq_pos = header.find('=');
    if (eq_pos != std::string::npos) {
        cookie.name = header.substr(0, eq_pos);
        // Trim whitespace
        cookie.name.erase(0, cookie.name.find_first_not_of(" \t"));
        cookie.name.erase(cookie.name.find_last_not_of(" \t") + 1);
        
        size_t semicolon_pos = header.find(';', eq_pos);
        if (semicolon_pos != std::string::npos) {
            cookie.value = header.substr(eq_pos + 1, semicolon_pos - eq_pos - 1);
        } else {
            cookie.value = header.substr(eq_pos + 1);
        }
        
        // Trim whitespace from value
        cookie.value.erase(0, cookie.value.find_first_not_of(" \t"));
        cookie.value.erase(cookie.value.find_last_not_of(" \t") + 1);
        
        pos = semicolon_pos + 1;
    }
    
    // Parse attributes
    while (pos < header.length()) {
        // Skip whitespace
        while (pos < header.length() && (header[pos] == ' ' || header[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= header.length()) {
            break;
        }
        
        // Find attribute name
        size_t attr_end = header.find('=', pos);
        if (attr_end == std::string::npos) {
            attr_end = header.find(';', pos);
            if (attr_end == std::string::npos) {
                attr_end = header.length();
            }
        }
        
        std::string attr_name = header.substr(pos, attr_end - pos);
        // Trim whitespace
        attr_name.erase(0, attr_name.find_first_not_of(" \t"));
        attr_name.erase(attr_name.find_last_not_of(" \t") + 1);
        
        std::transform(attr_name.begin(), attr_name.end(), attr_name.begin(), ::tolower);
        
        pos = attr_end;
        std::string attr_value;
        
        if (pos < header.length() && header[pos] == '=') {
            pos++;  // Skip '='
            size_t value_end = header.find(';', pos);
            if (value_end == std::string::npos) {
                value_end = header.length();
            }
            
            attr_value = header.substr(pos, value_end - pos);
            // Trim whitespace and quotes
            attr_value.erase(0, attr_value.find_first_not_of(" \t\""));
            attr_value.erase(attr_value.find_last_not_of(" \t\"") + 1);
            
            pos = value_end + 1;
        } else {
            pos = attr_end + 1;
        }
        
        // Process attribute
        if (attr_name == "domain") {
            cookie.domain = attr_value;
        } else if (attr_name == "path") {
            cookie.path = attr_value;
        } else if (attr_name == "expires") {
            // Parse date string (simplified)
            // In a real implementation, we would properly parse the date
            // For now, setting to a reasonable default
            cookie.expires = std::chrono::system_clock::now() + std::chrono::hours(24 * 30); // 30 days
        } else if (attr_name == "max-age") {
            try {
                int max_age = std::stoi(attr_value);
                cookie.expires = std::chrono::system_clock::now() + std::chrono::seconds(max_age);
            } catch (...) {
                // Invalid max-age, ignore
            }
        } else if (attr_name == "secure") {
            cookie.secure = true;
        } else if (attr_name == "httponly") {
            cookie.httponly = true;
        } else if (attr_name == "samesite") {
            cookie.samesite = attr_value;
        }
    }
    
    // If no domain was specified, use the origin domain
    if (cookie.domain.empty()) {
        cookie.domain = origin_domain;
    }
    
    // If no path was specified, derive from request path
    if (cookie.path.empty()) {
        cookie.path = "/";  // Default to root path
    }
    
    // Normalize domain
    cookie.domain = normalize_domain(cookie.domain);
    
    // Add the cookie to the jar
    add_cookie(cookie);
}

void CookieJar::clear_expired() {
    auto now = std::chrono::system_clock::now();
    cookies_.erase(
        std::remove_if(cookies_.begin(), cookies_.end(),
            [now](const Cookie& cookie) { return cookie.expires < now; }),
        cookies_.end()
    );
}

void CookieJar::clear() {
    cookies_.clear();
}

std::string CookieJar::normalize_domain(const std::string& domain) const {
    if (domain.empty()) {
        return domain;
    }
    
    std::string normalized = domain;
    if (normalized[0] != '.') {
        normalized = "." + normalized;
    }
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}

bool CookieJar::domain_matches(const std::string& cookie_domain, const std::string& request_domain) const {
    std::string norm_cookie = normalize_domain(cookie_domain);
    std::string norm_request = normalize_domain(request_domain);
    
    // Exact match
    if (norm_cookie == norm_request) {
        return true;
    }
    
    // Subdomain match (cookie domain is suffix of request domain)
    if (norm_request.length() > norm_cookie.length()) {
        if (norm_request.substr(norm_request.length() - norm_cookie.length()) == norm_cookie) {
            // Make sure the match is preceded by a dot (subdomain boundary)
            char preceding_char = norm_request[norm_request.length() - norm_cookie.length() - 1];
            if (preceding_char == '.') {
                return true;
            }
        }
    }
    
    return false;
}

bool CookieJar::save_to_file(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& cookie : cookies_) {
        // Serialize cookie to JSON-like format
        file << "{\n";
        file << "  \"name\": \"" << cookie.name << "\",\n";
        file << "  \"value\": \"" << cookie.value << "\",\n";
        file << "  \"domain\": \"" << cookie.domain << "\",\n";
        file << "  \"path\": \"" << cookie.path << "\",\n";
        
        // Convert time point to timestamp
        auto duration = cookie.expires.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        file << "  \"expires\": " << seconds << ",\n";
        
        file << "  \"secure\": " << (cookie.secure ? "true" : "false") << ",\n";
        file << "  \"httponly\": " << (cookie.httponly ? "true" : "false") << ",\n";
        file << "  \"samesite\": \"" << cookie.samesite << "\"\n";
        file << "},\n";
    }
    
    file.close();
    return true;
}

bool CookieJar::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Clear existing cookies
    cookies_.clear();
    
    // For simplicity, we'll just recreate the cookie jar
    // In a real implementation, you would parse the JSON properly
    file.close();
    return true;
}

bool CookieJar::save_as_mozilla_format(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header comment
    file << "# Netscape HTTP Cookie File\n";
    file << "# This is a generated file! Do not edit.\n\n";
    
    for (const auto& cookie : cookies_) {
        if (!cookie.is_expired()) {
            // Format: domain flag path secure expiration name value
            std::string domain_flag = (cookie.domain[0] == '.') ? "TRUE" : "FALSE";
            std::string secure_flag = cookie.secure ? "TRUE" : "FALSE";
            
            // Convert time point to timestamp
            auto duration = cookie.expires.time_since_epoch();
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            
            file << cookie.domain << "\t"
                 << domain_flag << "\t"
                 << cookie.path << "\t"
                 << secure_flag << "\t"
                 << seconds << "\t"
                 << cookie.name << "\t"
                 << cookie.value << "\n";
        }
    }
    
    file.close();
    return true;
}

bool CookieJar::load_from_mozilla_format(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    // Clear existing cookies
    cookies_.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "# ") {
            continue;
        }
        
        // Parse Mozilla cookie format: domain flag path secure expiration name value
        std::istringstream iss(line);
        std::string domain, flag, path, secure_str, expires_str, name, value;
        
        if (iss >> domain >> flag >> path >> secure_str >> expires_str >> name >> value) {
            Cookie cookie;
            cookie.domain = domain;
            cookie.path = path;
            cookie.name = name;
            cookie.value = value;
            cookie.secure = (secure_str == "TRUE");
            cookie.httponly = false; // Not specified in Mozilla format
            
            // Parse expiration time
            try {
                long long exp_time = std::stoll(expires_str);
                cookie.expires = std::chrono::system_clock::from_time_t(exp_time);
            } catch (...) {
                // If parsing fails, set to default expiration
                cookie.expires = std::chrono::system_clock::now() + std::chrono::hours(24 * 30);
            }
            
            cookies_.push_back(cookie);
        }
    }
    
    file.close();
    return true;
}

}  // namespace requests_cpp