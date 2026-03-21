#include "requests_cpp/streaming.hpp"
#include "requests_cpp/session.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

namespace requests_cpp {

// StreamingResponse 实现
void StreamingResponse::set_chunk_callback(StreamChunkCallback callback) {
    chunk_callback_ = std::move(callback);
}

void StreamingResponse::set_lazy_load(bool lazy_load) {
    lazy_load_ = lazy_load;
}

void StreamingResponse::set_chunk_size(size_t chunk_size) {
    chunk_size_ = chunk_size;
}

bool StreamingResponse::stream(const std::string& url, const std::string& method, const std::string& body) {
    if (!chunk_callback_) {
        return false;
    }

    // 这里模拟流式读取过程
    // 在实际实现中，这里会连接到服务器并逐块读取数据
    try {
        // 创建一个临时的session来发起请求
        Session session;
        
        // 根据方法类型发起请求
        Response response;
        if (method == "GET") {
            response = session.request("GET", url);
        } else if (method == "POST") {
            response = session.request("POST", url, body);
        } else {
            response = session.request(method, url, body);
        }
        
        // 模拟按块处理响应内容
        const std::string& content = response.text();
        size_t total_size = content.length();
        total_size_ = static_cast<long long>(total_size);
        downloaded_size_ = 0;
        
        // 按块处理内容
        for (size_t i = 0; i < total_size; i += chunk_size_) {
            size_t current_chunk_size = std::min(chunk_size_, total_size - i);
            std::string chunk = content.substr(i, current_chunk_size);
            
            // 调用回调函数处理块
            bool continue_reading = chunk_callback_(chunk);
            if (!continue_reading) {
                break;  // 用户请求停止读取
            }
            
            downloaded_size_ += current_chunk_size;
            
            // 模拟网络延迟
            if (lazy_load_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        // 在实际实现中，这里会处理网络错误
        return false;
    }
}

long long StreamingResponse::get_total_size() const {
    return total_size_;
}

long long StreamingResponse::get_downloaded_size() const {
    return downloaded_size_;
}

// HooksManager 实现
void HooksManager::add_pre_request_hook(PreRequestHook hook) {
    pre_request_hooks_.push_back(std::move(hook));
}

void HooksManager::add_post_response_hook(PostResponseHook hook) {
    post_response_hooks_.push_back(std::move(hook));
}

void HooksManager::add_error_hook(ErrorHook hook) {
    error_hooks_.push_back(std::move(hook));
}

void HooksManager::execute_pre_request_hooks(const std::string& url, const std::string& method) {
    for (const auto& hook : pre_request_hooks_) {
        try {
            hook(url, method);
        } catch (const std::exception& e) {
            // 记录钩子执行错误，但不中断主要流程
            std::cerr << "Pre-request hook error: " << e.what() << std::endl;
        }
    }
}

void HooksManager::execute_post_response_hooks(int status_code, const std::string& url) {
    for (const auto& hook : post_response_hooks_) {
        try {
            hook(status_code, url);
        } catch (const std::exception& e) {
            // 记录钩子执行错误，但不中断主要流程
            std::cerr << "Post-response hook error: " << e.what() << std::endl;
        }
    }
}

void HooksManager::execute_error_hooks(const std::string& error, const std::string& url) {
    for (const auto& hook : error_hooks_) {
        try {
            hook(error, url);
        } catch (const std::exception& e) {
            // 记录钩子执行错误，但不中断主要流程
            std::cerr << "Error hook error: " << e.what() << std::endl;
        }
    }
}

// LogManager 实现
void LogManager::set_log_level(LogLevel level) {
    log_level_ = level;
}

void LogManager::set_log_callback(LogCallback callback) {
    log_callback_ = std::move(callback);
}

void LogManager::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void LogManager::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void LogManager::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void LogManager::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void LogManager::log(LogLevel level, const std::string& message) {
    if (!should_log(level)) {
        return;
    }
    
    if (log_callback_) {
        try {
            log_callback_(level, message);
        } catch (const std::exception& e) {
            // 如果回调本身出错，至少尝试输出到标准错误
            std::cerr << "Log callback error: " << e.what() << std::endl;
        }
    } else {
        // 默认日志输出到控制台
        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO: level_str = "INFO"; break;
            case LogLevel::WARN: level_str = "WARN"; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
            default: level_str = "UNKNOWN"; break;
        }
        
        std::cout << "[" << level_str << "] " << message << std::endl;
    }
}

bool LogManager::should_log(LogLevel level) const {
    return level >= log_level_ && level != LogLevel::NONE;
}

// AdvancedSession 实现
AdvancedSession::AdvancedSession() 
    : streaming_response_(std::make_unique<StreamingResponse>())
    , hooks_manager_(std::make_unique<HooksManager>())
    , log_manager_(std::make_unique<LogManager>()) {
}

void AdvancedSession::set_stream_chunk_callback(StreamChunkCallback callback) {
    streaming_response_->set_chunk_callback(std::move(callback));
}

void AdvancedSession::add_pre_request_hook(PreRequestHook hook) {
    hooks_manager_->add_pre_request_hook(std::move(hook));
}

void AdvancedSession::add_post_response_hook(PostResponseHook hook) {
    hooks_manager_->add_post_response_hook(std::move(hook));
}

void AdvancedSession::add_error_hook(ErrorHook hook) {
    hooks_manager_->add_error_hook(std::move(hook));
}

void AdvancedSession::set_log_level(LogLevel level) {
    log_manager_->set_log_level(level);
}

void AdvancedSession::set_log_callback(LogCallback callback) {
    log_manager_->set_log_callback(std::move(callback));
}

std::string AdvancedSession::get(const std::string& url, bool stream_chunks) {
    log_request(url, "GET");
    
    try {
        hooks_manager_->execute_pre_request_hooks(url, "GET");
        
        if (stream_chunks) {
            // 使用流式读取
            bool success = streaming_response_->stream(url, "GET");
            if (!success) {
                std::string error_msg = "Failed to stream GET request: " + url;
                log_error(url, error_msg);
                hooks_manager_->execute_error_hooks(error_msg, url);
                return "";
            }
            return "Streamed successfully";
        } else {
            // 使用普通请求
            Session session;
            Response response = session.request("GET", url);
            
            hooks_manager_->execute_post_response_hooks(response.status_code(), url);
            log_response(url, response.status_code());
            
            return response.text();
        }
    } catch (const std::exception& e) {
        std::string error_msg = std::string("GET request failed: ") + e.what();
        log_error(url, error_msg);
        hooks_manager_->execute_error_hooks(error_msg, url);
        return "";
    }
}

std::string AdvancedSession::post(const std::string& url, const std::string& body, bool stream_chunks) {
    log_request(url, "POST");
    
    try {
        hooks_manager_->execute_pre_request_hooks(url, "POST");
        
        if (stream_chunks) {
            // 使用流式读取
            bool success = streaming_response_->stream(url, "POST", body);
            if (!success) {
                std::string error_msg = "Failed to stream POST request: " + url;
                log_error(url, error_msg);
                hooks_manager_->execute_error_hooks(error_msg, url);
                return "";
            }
            return "Streamed successfully";
        } else {
            // 使用普通请求
            Session session;
            Response response = session.request("POST", url, body);
            
            hooks_manager_->execute_post_response_hooks(response.status_code(), url);
            log_response(url, response.status_code());
            
            return response.text();
        }
    } catch (const std::exception& e) {
        std::string error_msg = std::string("POST request failed: ") + e.what();
        log_error(url, error_msg);
        hooks_manager_->execute_error_hooks(error_msg, url);
        return "";
    }
}

void AdvancedSession::log_request(const std::string& url, const std::string& method) {
    if (log_manager_) {
        log_manager_->debug("Sending " + method + " request to: " + url);
    }
}

void AdvancedSession::log_response(const std::string& url, int status_code) {
    if (log_manager_) {
        log_manager_->info("Received response " + std::to_string(status_code) + " from: " + url);
    }
}

void AdvancedSession::log_error(const std::string& url, const std::string& error) {
    if (log_manager_) {
        log_manager_->error("Error for " + url + ": " + error);
    }
}

}  // namespace requests_cpp