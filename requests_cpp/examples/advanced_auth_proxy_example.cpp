#include "requests_cpp/session.hpp"
#include "requests_cpp/auth.hpp"
#include "requests_cpp/cookie.hpp"
#include "requests_cpp/proxy.hpp"
#include "requests_cpp/retry.hpp"

#include <iostream>

int main() {
    try {
        requests_cpp::Session session;
        
        // Example 1: Enhanced authentication
        std::cout << "Setting up enhanced authentication..." << std::endl;
        
        // Set up NTLM authentication
        session.set_ntlm_auth("username", "password", "domain", "workstation");
        
        // Or continue with other auth methods
        // session.set_digest_auth("username", "password");
        
        // Example 2: Advanced proxy configuration
        std::cout << "Setting up advanced proxy configuration..." << std::endl;
        requests_cpp::ProxyConfig proxy_config;
        proxy_config.http_proxy = "http://proxy.example.com:8080";
        proxy_config.https_proxy = "https://proxy.example.com:8080";
        proxy_config.socks5_proxy = "socks5://socks-proxy.example.com:1080";
        session.set_proxy_config(proxy_config);
        
        // Example 3: Cookie persistence
        std::cout << "Working with persistent cookies..." << std::endl;
        requests_cpp::CookieJar& cookie_jar = session.cookies();
        
        // Add a sample cookie
        requests_cpp::Cookie cookie;
        cookie.name = "persistent_session";
        cookie.value = "abc123xyz";
        cookie.domain = ".example.com";
        cookie.path = "/";
        cookie.secure = true;
        cookie.httponly = true;
        cookie.expires = std::chrono::system_clock::now() + std::chrono::hours(24 * 30); // 30 days
        cookie_jar.add_cookie(cookie);
        
        // Save cookies to file
        cookie_jar.save_to_file("cookies.json");
        std::cout << "Cookies saved to cookies.json" << std::endl;
        
        // Save cookies in Mozilla format
        cookie_jar.save_as_mozilla_format("cookies.txt");
        std::cout << "Cookies saved to cookies.txt (Mozilla format)" << std::endl;
        
        // Example 4: Making a request with all features
        std::cout << "Making request with all new features..." << std::endl;
        auto response = session.get("https://httpbin.org/get");
        
        if (response.status_code() == 200) {
            std::cout << "Success! Response: " << response.text().substr(0, 100) << "..." << std::endl;
        } else {
            std::cout << "Request failed with status: " << response.status_code() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}