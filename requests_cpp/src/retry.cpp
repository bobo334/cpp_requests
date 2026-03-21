#include "requests_cpp/retry.hpp"
#include <random>
#include <algorithm>
#include <cctype>

namespace requests_cpp {

bool RetryConfig::should_retry_status_code(int status_code) const {
    for (int code : status_codes_to_retry) {
        if (code == status_code) {
            return true;
        }
    }
    return false;
}

bool RetryConfig::is_idempotent_method(const std::string& method) {
    // Idempotent methods according to HTTP specification
    std::string upper_method = method;
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(), ::toupper);
    
    return upper_method == "GET" || 
           upper_method == "HEAD" || 
           upper_method == "PUT" || 
           upper_method == "DELETE" || 
           upper_method == "OPTIONS" || 
           upper_method == "TRACE";
}

RetryStrategy::RetryStrategy(const RetryConfig& config) : config_(config) {}

std::chrono::milliseconds RetryStrategy::calculate_delay(int attempt_count) const {
    // Exponential backoff with jitter
    if (attempt_count <= 0) {
        return std::chrono::milliseconds(0);
    }
    
    // Calculate base delay: backoff_factor * (2 ^ (attempt_count - 1))
    long long base_delay = config_.backoff_factor.count();
    for (int i = 0; i < attempt_count - 1; ++i) {
        base_delay *= 2;
    }
    
    // Apply jitter: add random value between 0 and base_delay*0.1
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<long long> dis(0, static_cast<long long>(base_delay * 0.1));
    
    long long jitter = dis(gen);
    long long total_delay = base_delay + jitter;
    
    // Cap at max_backoff
    if (total_delay > config_.max_backoff.count()) {
        total_delay = config_.max_backoff.count();
    }
    
    return std::chrono::milliseconds(total_delay);
}

bool RetryStrategy::should_retry_status(int status_code) const {
    return config_.should_retry_status_code(status_code);
}

bool RetryStrategy::should_retry_error(const std::string& error_type) const {
    // Define retryable error types
    return error_type == "connection_error" || 
           error_type == "timeout_error" || 
           error_type == "ssl_error" ||
           error_type == "network_error";
}

bool RetryStrategy::is_safe_to_retry_method(const std::string& method) const {
    return RetryConfig::is_idempotent_method(method);
}

}  // namespace requests_cpp