# API Documentation

## Table of Contents
- [Request Class](#request-class)
- [Response Class](#response-class)
- [Session Class](#session-class)
- [Authentication](#authentication)
- [Proxy Support](#proxy-support)
- [TLS Configuration](#tls-configuration)
- [Streaming Features](#streaming-features)
- [Hooks System](#hooks-system)
- [Logging](#logging)

## Request Class

The `Request` class represents an HTTP request.

### Constructor
```cpp
Request() = default;
explicit Request(std::string url);
```

### Methods
- `const std::string& url() const` - Get the request URL
- `void set_url(std::string url)` - Set the request URL
- `const std::string& method() const` - Get the HTTP method
- `void set_method(std::string method)` - Set the HTTP method
- `const std::string& body() const` - Get the request body
- `void set_body(std::string body)` - Set the request body
- `const Headers& headers() const` - Get the request headers
- `void set_headers(Headers headers)` - Set the request headers

## Response Class

The `Response` class represents an HTTP response.

### Methods
- `int status_code() const` - Get the HTTP status code
- `void set_status_code(int status_code)` - Set the HTTP status code
- `const std::string& text() const` - Get the response body as text
- `void set_text(std::string text)` - Set the response body as text
- `const Headers& headers() const` - Get the response headers
- `void set_headers(Headers headers)` - Set the response headers

## Session Class

The `Session` class provides a high-level interface for making HTTP requests.

### Methods
- `Response request(const std::string& method, const std::string& url)` - Make an HTTP request
- `Response get(const std::string& url)` - Make a GET request
- `Response post(const std::string& url)` - Make a POST request
- `Response post(const std::string& url, const std::string& body)` - Make a POST request with body
- `void set_proxy_config(const ProxyConfig& config)` - Configure proxy settings
- `void set_auth(std::shared_ptr<Auth> auth)` - Set authentication method
- `void set_basic_auth(const std::string& username, const std::string& password)` - Set basic authentication
- `void set_bearer_token_auth(const std::string& token)` - Set bearer token authentication

## Authentication

### Basic Authentication
```cpp
#include "requests_cpp/auth.hpp"

requests_cpp::BasicAuth auth("username", "password");
```

### Bearer Token Authentication
```cpp
requests_cpp::BearerTokenAuth auth("token");
```

### Digest Authentication
```cpp
requests_cpp::DigestAuth auth("username", "password");
```

### NTLM Authentication
```cpp
requests_cpp::NtlmAuth auth("username", "password", "domain", "workstation");
```

## Proxy Support

### Proxy Configuration
```cpp
requests_cpp::ProxyConfig config;
config.http_proxy = "http://proxy.company.com:8080";
config.https_proxy = "https://proxy.company.com:8080";
config.socks5_proxy = "socks5://socks-proxy.company.com:1080";

requests_cpp::ProxyManager manager(config);
```

## TLS Configuration

### Basic TLS Configuration
```cpp
requests_cpp::tls::TlsConfig config;
config.verify = true;
config.ca_path = "/path/to/ca-bundle.crt";
config.connect_timeout_ms = 10000;
```

### Enhanced TLS Configuration
```cpp
#include "requests_cpp/tls_enhanced.hpp"

requests_cpp::tls::EnhancedTlsConfig config;
config.pinning_options.enabled = true;
config.pinning_options.public_key_hashes = {"sha256_hash_here"};
config.hostname_verification_options.allowed_hostnames = {"*.example.com"};
```

## Streaming Features

### Streaming Response
```cpp
#include "requests_cpp/streaming.hpp"

requests_cpp::StreamingResponse stream_resp;
stream_resp.set_chunk_callback([](const std::string& chunk) -> bool {
    std::cout << "Received chunk: " << chunk.length() << " bytes" << std::endl;
    return true; // Continue reading
});
stream_resp.set_chunk_size(8192); // 8KB chunks
```

## Hooks System

### Adding Hooks
```cpp
requests_cpp::AdvancedSession session;

// Pre-request hook
session.add_pre_request_hook([](const std::string& url, const std::string& method) {
    std::cout << "About to make " << method << " request to " << url << std::endl;
});

// Post-response hook
session.add_post_response_hook([](int status_code, const std::string& url) {
    std::cout << "Received status " << status_code << " from " << url << std::endl;
});

// Error hook
session.add_error_hook([](const std::string& error, const std::string& url) {
    std::cout << "Error for " << url << ": " << error << std::endl;
});
```

## Logging

### Setting Up Logging
```cpp
session.set_log_level(requests_cpp::LogLevel::INFO);
session.set_log_callback([](requests_cpp::LogLevel level, const std::string& message) {
    std::cout << "[" << static_cast<int>(level) << "] " << message << std::endl;
});