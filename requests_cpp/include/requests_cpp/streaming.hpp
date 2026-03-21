#pragma once

#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <iostream>

#include "requests_cpp/export.hpp"

namespace requests_cpp {

// 流式读取回调类型定义
using StreamChunkCallback = std::function<bool(const std::string& chunk)>;

// Hooks回调类型定义
using PreRequestHook = std::function<void(const std::string& url, const std::string& method)>;
using PostResponseHook = std::function<void(int status_code, const std::string& url)>;
using ErrorHook = std::function<void(const std::string& error, const std::string& url)>;

// 日志级别枚举
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    NONE = 4
};

// 日志回调类型定义
using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

/**
 * 流式响应处理器
 * 支持按块读取响应内容
 */
class REQUESTS_CPP_API StreamingResponse {
public:
    StreamingResponse() = default;
    
    /**
     * 设置流式读取回调函数
     * @param callback 每接收到一块数据时调用的回调函数
     *                 返回true继续读取，返回false停止读取
     */
    void set_chunk_callback(StreamChunkCallback callback);
    
    /**
     * 设置延迟加载标志
     * @param lazy_load 是否启用延迟加载
     */
    void set_lazy_load(bool lazy_load);
    
    /**
     * 设置块大小
     * @param chunk_size 每次读取的块大小（字节）
     */
    void set_chunk_size(size_t chunk_size);
    
    /**
     * 开始流式读取
     * @param url 请求的URL
     * @param method HTTP方法
     * @param body 请求体（可选）
     * @return 是否成功开始流式读取
     */
    bool stream(const std::string& url, const std::string& method = "GET", 
               const std::string& body = "");
    
    /**
     * 获取总大小（如果可用）
     * @return 内容总大小，如果不可用则返回-1
     */
    long long get_total_size() const;
    
    /**
     * 获取已读取大小
     * @return 已读取的字节数
     */
    long long get_downloaded_size() const;

private:
    StreamChunkCallback chunk_callback_;
    bool lazy_load_{false};
    size_t chunk_size_{8192};  // 默认8KB块大小
    long long total_size_{-1};
    long long downloaded_size_{0};
};

/**
 * Hooks管理器
 * 提供请求前、响应后和错误处理的回调机制
 */
class REQUESTS_CPP_API HooksManager {
public:
    HooksManager() = default;
    
    /**
     * 添加请求前钩子
     * @param hook 在请求发送前调用的回调函数
     */
    void add_pre_request_hook(PreRequestHook hook);
    
    /**
     * 添加响应后钩子
     * @param hook 在收到响应后调用的回调函数
     */
    void add_post_response_hook(PostResponseHook hook);
    
    /**
     * 添加错误钩子
     * @param hook 在发生错误时调用的回调函数
     */
    void add_error_hook(ErrorHook hook);
    
    /**
     * 执行请求前钩子
     * @param url 请求的URL
     * @param method HTTP方法
     */
    void execute_pre_request_hooks(const std::string& url, const std::string& method);
    
    /**
     * 执行响应后钩子
     * @param status_code 响应状态码
     * @param url 请求的URL
     */
    void execute_post_response_hooks(int status_code, const std::string& url);
    
    /**
     * 执行错误钩子
     * @param error 错误信息
     * @param url 请求的URL
     */
    void execute_error_hooks(const std::string& error, const std::string& url);

private:
    std::vector<PreRequestHook> pre_request_hooks_;
    std::vector<PostResponseHook> post_response_hooks_;
    std::vector<ErrorHook> error_hooks_;
};

/**
 * 日志管理器
 * 提供可配置的日志记录功能
 */
class REQUESTS_CPP_API LogManager {
public:
    LogManager() = default;
    
    /**
     * 设置日志级别
     * @param level 日志级别
     */
    void set_log_level(LogLevel level);
    
    /**
     * 设置日志回调
     * @param callback 日志输出回调函数
     */
    void set_log_callback(LogCallback callback);
    
    /**
     * 记录DEBUG级别的日志
     * @param message 日志消息
     */
    void debug(const std::string& message);
    
    /**
     * 记录INFO级别的日志
     * @param message 日志消息
     */
    void info(const std::string& message);
    
    /**
     * 记录WARN级别的日志
     * @param message 日志消息
     */
    void warn(const std::string& message);
    
    /**
     * 记录ERROR级别的日志
     * @param message 日志消息
     */
    void error(const std::string& message);
    
    /**
     * 记录指定级别的日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const std::string& message);

private:
    LogLevel log_level_{LogLevel::INFO};
    LogCallback log_callback_{nullptr};
    
    /**
     * 检查是否应该记录指定级别的日志
     * @param level 日志级别
     * @return 是否应该记录
     */
    bool should_log(LogLevel level) const;
};

/**
 * 高级会话类
 * 集成了流式读取、Hooks和日志功能
 */
class REQUESTS_CPP_API AdvancedSession {
public:
    AdvancedSession();
    
    /**
     * 设置流式读取回调
     * @param callback 流式读取回调函数
     */
    void set_stream_chunk_callback(StreamChunkCallback callback);
    
    /**
     * 添加请求前钩子
     * @param hook 请求前钩子函数
     */
    void add_pre_request_hook(PreRequestHook hook);
    
    /**
     * 添加响应后钩子
     * @param hook 响应后钩子函数
     */
    void add_post_response_hook(PostResponseHook hook);
    
    /**
     * 添加错误钩子
     * @param hook 错误钩子函数
     */
    void add_error_hook(ErrorHook hook);
    
    /**
     * 设置日志级别
     * @param level 日志级别
     */
    void set_log_level(LogLevel level);
    
    /**
     * 设置日志回调
     * @param callback 日志回调函数
     */
    void set_log_callback(LogCallback callback);
    
    /**
     * 执行GET请求（支持流式读取）
     * @param url 请求URL
     * @param stream_chunks 是否使用流式读取
     * @return 响应对象
     */
    std::string get(const std::string& url, bool stream_chunks = false);
    
    /**
     * 执行POST请求（支持流式读取）
     * @param url 请求URL
     * @param body 请求体
     * @param stream_chunks 是否使用流式读取
     * @return 响应对象
     */
    std::string post(const std::string& url, const std::string& body = "", bool stream_chunks = false);

private:
    std::unique_ptr<StreamingResponse> streaming_response_;
    std::unique_ptr<HooksManager> hooks_manager_;
    std::unique_ptr<LogManager> log_manager_;
    
    /**
     * 记录请求相关信息
     * @param url 请求URL
     * @param method HTTP方法
     */
    void log_request(const std::string& url, const std::string& method);
    
    /**
     * 记录响应相关信息
     * @param url 请求URL
     * @param status_code 响应状态码
     */
    void log_response(const std::string& url, int status_code);
    
    /**
     * 记录错误信息
     * @param url 请求URL
     * @param error 错误信息
     */
    void log_error(const std::string& url, const std::string& error);
};

}  // namespace requests_cpp