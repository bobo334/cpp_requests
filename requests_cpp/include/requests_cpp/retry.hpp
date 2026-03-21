#pragma once

#include <functional>
#include <chrono>
#include <vector>

namespace requests_cpp {

struct RetryConfig {
    int total_retries{3};
    int connect_retries{3};
    int read_retries{3};
    int status_retries{3};
    std::vector<int> status_codes_to_retry{413, 429, 503};  // Common retryable status codes
    std::chrono::milliseconds backoff_factor{1000};  // 1 second base delay
    std::chrono::milliseconds max_backoff{10000};    // 10 seconds max delay
    bool respect_retry_after_header{true};
    
    // Check if a status code should trigger a retry
    bool should_retry_status_code(int status_code) const;
    
    // Check if a method is considered safe for retrying
    static bool is_idempotent_method(const std::string& method);
};

class RetryStrategy {
public:
    RetryStrategy() = default;
    explicit RetryStrategy(const RetryConfig& config);
    
    // Calculate delay before next retry using exponential backoff
    std::chrono::milliseconds calculate_delay(int attempt_count) const;
    
    // Check if we should retry based on status code
    bool should_retry_status(int status_code) const;
    
    // Check if we should retry based on error condition
    bool should_retry_error(const std::string& error_type) const;
    
    // Check if method is safe to retry
    bool is_safe_to_retry_method(const std::string& method) const;
    
private:
    RetryConfig config_;
};

}  // namespace requests_cpp