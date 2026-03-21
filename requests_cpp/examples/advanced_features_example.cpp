#include "requests_cpp/streaming.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== Advanced Features Example ===" << std::endl;

    std::cout << "\n--- Streaming ---" << std::endl;
    try {
        requests_cpp::AdvancedSession session;
        session.set_stream_chunk_callback([](const std::string& chunk) {
            std::cout << "Chunk bytes: " << chunk.size() << std::endl;
            return true;
        });

        auto streaming_response = std::make_unique<requests_cpp::StreamingResponse>();
        streaming_response->set_chunk_size(1024);
        std::cout << "Streaming callback is ready" << std::endl;
    } catch (const std::exception& error) {
        std::cout << "Streaming example error: " << error.what() << std::endl;
    }

    std::cout << "\n--- Hooks ---" << std::endl;
    try {
        requests_cpp::AdvancedSession session;
        session.add_pre_request_hook([](const std::string& url, const std::string& method) {
            std::cout << "Before request: " << method << " " << url << std::endl;
        });
        session.add_post_response_hook([](int status_code, const std::string& url) {
            std::cout << "After response: " << url << " => " << status_code << std::endl;
        });
        session.add_error_hook([](const std::string& error, const std::string& url) {
            std::cout << "Request error: " << url << " => " << error << std::endl;
        });

        std::cout << "Hooks are ready" << std::endl;
    } catch (const std::exception& error) {
        std::cout << "Hooks example error: " << error.what() << std::endl;
    }

    std::cout << "\n--- Logging ---" << std::endl;
    try {
        requests_cpp::AdvancedSession session;
        session.set_log_level(requests_cpp::LogLevel::DEBUG);
        session.set_log_callback([](requests_cpp::LogLevel level, const std::string& message) {
            std::string level_text = "UNKNOWN";
            if (level == requests_cpp::LogLevel::DEBUG) level_text = "DEBUG";
            if (level == requests_cpp::LogLevel::INFO) level_text = "INFO";
            if (level == requests_cpp::LogLevel::WARN) level_text = "WARN";
            if (level == requests_cpp::LogLevel::ERROR) level_text = "ERROR";

            std::cout << "[" << std::setw(5) << level_text << "] " << message << std::endl;

            std::ofstream file("requests.log", std::ios::app);
            if (file.is_open()) file << "[" << level_text << "] " << message << std::endl;
        });

        auto log_manager = std::make_unique<requests_cpp::LogManager>();
        log_manager->set_log_level(requests_cpp::LogLevel::DEBUG);
        log_manager->set_log_callback([](requests_cpp::LogLevel level, const std::string& message) {
            std::string level_text = "UNKNOWN";
            if (level == requests_cpp::LogLevel::DEBUG) level_text = "DEBUG";
            if (level == requests_cpp::LogLevel::INFO) level_text = "INFO";
            if (level == requests_cpp::LogLevel::WARN) level_text = "WARN";
            if (level == requests_cpp::LogLevel::ERROR) level_text = "ERROR";
            std::cout << "[" << level_text << "] " << message << std::endl;
        });

        log_manager->debug("debug message");
        log_manager->info("info message");
        log_manager->warn("warn message");
        log_manager->error("error message");
    } catch (const std::exception& error) {
        std::cout << "Logging example error: " << error.what() << std::endl;
    }

    std::cout << "\n=== Advanced Features Example Done ===" << std::endl;
    return 0;
}
