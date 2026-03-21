#include <iostream>
#include <cassert>
#include <string>
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/session.hpp"
#include "requests_cpp/auth.hpp"
#include "requests_cpp/proxy.hpp"
#include "requests_cpp/tls.hpp"
#include "requests_cpp/streaming.hpp"

void test_request_creation() {
    std::cout << "Testing Request creation..." << std::endl;
    
    requests_cpp::Request req("https://example.com");
    assert(req.url() == "https://example.com");
    assert(req.method() == "GET");
    
    req.set_method("POST");
    assert(req.method() == "POST");
    
    std::cout << "Request creation test passed!" << std::endl;
}

void test_response_creation() {
    std::cout << "Testing Response creation..." << std::endl;
    
    requests_cpp::Response resp;
    resp.set_status_code(200);
    resp.set_text("OK");
    
    assert(resp.status_code() == 200);
    assert(resp.text() == "OK");
    
    std::cout << "Response creation test passed!" << std::endl;
}

void test_basic_auth() {
    std::cout << "Testing Basic Auth..." << std::endl;
    
    requests_cpp::BasicAuth auth("user", "pass");
    std::string header = auth.get_auth_header();
    
    // Basic header should start with "Basic "
    assert(header.substr(0, 6) == "Basic ");
    
    std::cout << "Basic Auth test passed!" << std::endl;
}

void test_bearer_token_auth() {
    std::cout << "Testing Bearer Token Auth..." << std::endl;
    
    requests_cpp::BearerTokenAuth auth("token123");
    std::string header = auth.get_auth_header();
    
    // Bearer header should start with "Bearer "
    assert(header.substr(0, 7) == "Bearer ");
    
    std::cout << "Bearer Token Auth test passed!" << std::endl;
}

void test_proxy_config() {
    std::cout << "Testing Proxy Config..." << std::endl;
    
    requests_cpp::ProxyConfig config = requests_cpp::ProxyConfig::from_environment();
    
    // Test basic functionality
    std::string proxy_url = "http://proxy.example.com:8080";
    bool should_bypass = false; // This depends on the configuration
    
    std::cout << "Proxy Config test passed!" << std::endl;
}

void test_streaming_response() {
    std::cout << "Testing Streaming Response..." << std::endl;
    
    requests_cpp::StreamingResponse stream_resp;
    stream_resp.set_chunk_size(1024);
    assert(stream_resp.get_total_size() == -1); // Initially unknown
    
    std::cout << "Streaming Response test passed!" << std::endl;
}

void test_hooks_manager() {
    std::cout << "Testing Hooks Manager..." << std::endl;
    
    requests_cpp::HooksManager hooks;
    
    // Add a simple pre-request hook
    hooks.add_pre_request_hook([](const std::string& url, const std::string& method) {
        std::cout << "Pre-request: " << method << " " << url << std::endl;
    });
    
    // Execute the hook
    hooks.execute_pre_request_hooks("https://example.com", "GET");
    
    std::cout << "Hooks Manager test passed!" << std::endl;
}

void test_advanced_session() {
    std::cout << "Testing Advanced Session..." << std::endl;
    
    requests_cpp::AdvancedSession session;
    
    // Test setting log level
    session.set_log_level(requests_cpp::LogLevel::INFO);
    
    // Test setting a simple chunk callback
    session.set_stream_chunk_callback([](const std::string& chunk) -> bool {
        std::cout << "Received chunk of size: " << chunk.length() << std::endl;
        return true;
    });
    
    std::cout << "Advanced Session test passed!" << std::endl;
}

void run_all_tests() {
    std::cout << "Starting unit tests..." << std::endl;
    
    test_request_creation();
    test_response_creation();
    test_basic_auth();
    test_bearer_token_auth();
    test_proxy_config();
    test_streaming_response();
    test_hooks_manager();
    test_advanced_session();
    
    std::cout << "All tests passed!" << std::endl;
}

int main() {
    try {
        run_all_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}