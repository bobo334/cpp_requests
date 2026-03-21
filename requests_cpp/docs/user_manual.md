# User Manual

## Getting Started

### Installation

#### Using vcpkg
```bash
vcpkg install requests-cpp
```

#### Using Conan
```bash
conan install requests-cpp/0.1.0@
```

#### Building from Source
```bash
git clone https://github.com/requests-cpp/requests-cpp.git
cd requests-cpp
mkdir build && cd build
cmake ..
make
```

### Basic Usage

#### Making a Simple GET Request
```cpp
#include "requests_cpp/session.hpp"
#include <iostream>

int main() {
    requests_cpp::Session session;
    requests_cpp::Response response = session.get("https://httpbin.org/get");
    
    std::cout << "Status: " << response.status_code() << std::endl;
    std::cout << "Body: " << response.text() << std::endl;
    
    return 0;
}
```

#### Making a POST Request
```cpp
#include "requests_cpp/session.hpp"
#include <iostream>

int main() {
    requests_cpp::Session session;
    requests_cpp::Response response = session.post("https://httpbin.org/post", "Hello, World!");
    
    std::cout << "Status: " << response.status_code() << std::endl;
    std::cout << "Body: " << response.text() << std::endl;
    
    return 0;
}
```

## Advanced Features

### Authentication

#### Basic Authentication
```cpp
#include "requests_cpp/session.hpp"
#include "requests_cpp/auth.hpp"

requests_cpp::Session session;
session.set_basic_auth("username", "password");

requests_cpp::Response response = session.get("https://httpbin.org/basic-auth/user/pass");
```

#### Bearer Token Authentication
```cpp
session.set_bearer_token_auth("your-token-here");
```

#### Custom Authentication
```cpp
std::shared_ptr<requests_cpp::Auth> custom_auth = std::make_shared<CustomAuth>("params");
session.set_auth(custom_auth);
```

### Proxy Configuration

#### Setting Up Proxies
```cpp
#include "requests_cpp/proxy.hpp"

requests_cpp::Session session;
requests_cpp::ProxyConfig proxy_config;
proxy_config.http_proxy = "http://proxy.company.com:8080";
proxy_config.https_proxy = "https://proxy.company.com:8080";

session.set_proxy_config(proxy_config);
```

### TLS Configuration

#### Basic TLS Settings
```cpp
#include "requests_cpp/session.hpp"

requests_cpp::Session session;
// TLS verification is enabled by default
// To disable (not recommended for production):
// session.set_verify(false);
```

#### Advanced TLS with Certificate Pinning
```cpp
#include "requests_cpp/tls_enhanced.hpp"

requests_cpp::tls::EnhancedTlsConfig tls_config;
tls_config.pinning_options.enabled = true;
tls_config.pinning_options.public_key_hashes = {
    "sha256_hash_of_server_public_key"
};

// Use with session...
```

### Streaming Large Responses

#### Processing Large Files in Chunks
```cpp
#include "requests_cpp/streaming.hpp"

requests_cpp::AdvancedSession session;
session.set_stream_chunk_callback([](const std::string& chunk) -> bool {
    // Process the chunk (e.g., write to file, analyze data)
    std::cout << "Processing chunk of size: " << chunk.length() << std::endl;
    
    // Return true to continue, false to stop
    return true;
});

// This will stream the response in chunks
session.get("https://httpbin.org/stream-bytes/1000000", true); // 1MB file
```

### Hooks System

#### Adding Request/Response Hooks
```cpp
#include "requests_cpp/streaming.hpp"

requests_cpp::AdvancedSession session;

// Hook called before each request
session.add_pre_request_hook([](const std::string& url, const std::string& method) {
    std::cout << "Making " << method << " request to " << url << std::endl;
});

// Hook called after each response
session.add_post_response_hook([](int status_code, const std::string& url) {
    std::cout << "Got response " << status_code << " from " << url << std::endl;
});

// Hook called on errors
session.add_error_hook([](const std::string& error, const std::string& url) {
    std::cerr << "Error requesting " << url << ": " << error << std::endl;
});
```

### Logging

#### Setting Up Logging
```cpp
#include "requests_cpp/streaming.hpp"

requests_cpp::AdvancedSession session;

// Set log level
session.set_log_level(requests_cpp::LogLevel::INFO);

// Set custom log handler
session.set_log_callback([](requests_cpp::LogLevel level, const std::string& message) {
    // Custom logging logic
    std::cout << "[" << static_cast<int>(level) << "] " << message << std::endl;
});
```

## Configuration Options

### Build Options

- `BUILD_SHARED_LIBS`: Build shared libraries instead of static
- `REQUESTS_CPP_BUILD_EXAMPLES`: Build example programs
- `REQUESTS_CPP_BUILD_TESTS`: Build test programs

### Runtime Configuration

#### Timeouts
```cpp
requests_cpp::Request request("https://httpbin.org/delay/10");
request.set_timeout({/* connect timeout */, /* read timeout */});
```

#### Retries
```cpp
request.set_retries(3); // Retry up to 3 times
```

## Error Handling

### Common Error Types
- `requests_cpp::RequestException`: General request errors
- `requests_cpp::ConnectionException`: Connection-related errors
- `requests_cpp::SSLError`: SSL/TLS-related errors
- `requests_cpp::ProxyError`: Proxy-related errors

### Error Checking
```cpp
try {
    requests_cpp::Response response = session.get("https://httpbin.org/get");
    if (response.error_code() != requests_cpp::ErrorCode::None) {
        std::cerr << "Request failed: " << response.error_code() << std::endl;
    }
} catch (const requests_cpp::RequestException& e) {
    std::cerr << "Request exception: " << e.what() << std::endl;
}
```

## Performance Tips

### Connection Pooling
The library automatically manages connection pooling. Reuse sessions for multiple requests to the same host to benefit from connection reuse.

### Streaming for Large Data
Use streaming APIs when dealing with large responses to avoid loading everything into memory at once.

### Parallel Requests
Create multiple session objects for truly parallel requests, or use the async capabilities if available in future versions.

## Troubleshooting

### Common Issues
1. **SSL Certificate Verification Failures**: Check your CA bundle path or disable verification for testing only
2. **Proxy Not Working**: Verify proxy URL format and authentication if required
3. **Timeout Errors**: Increase timeout values for slow networks or heavy loads

### Debugging
Enable logging with `LogLevel::DEBUG` to see detailed request/response information.