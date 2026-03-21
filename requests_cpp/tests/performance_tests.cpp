#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <future>
#include "requests_cpp/session.hpp"
#include "requests_cpp/request.hpp"

void benchmark_simple_requests() {
    std::cout << "Benchmarking simple requests..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform multiple simple requests
    for (int i = 0; i < 100; ++i) {
        // This would normally make actual HTTP requests
        // For this test, we'll just simulate the operation
        volatile int dummy = i * i; // Prevent optimization
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "100 simulated requests took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average per request: " << duration.count() / 100.0 << " microseconds" << std::endl;
}

void benchmark_concurrent_requests() {
    std::cout << "Benchmarking concurrent requests..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate concurrent requests using threads
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(std::async(std::launch::async, []() {
            // Simulate work for each concurrent request
            for (int j = 0; j < 10; ++j) {
                volatile int dummy = j * j; // Prevent optimization
            }
        }));
    }
    
    // Wait for all futures to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "10 concurrent batches (100 total) took: " << duration.count() << " microseconds" << std::endl;
}

void benchmark_large_payloads() {
    std::cout << "Benchmarking large payload handling..." << std::endl;
    
    // Create a large payload simulation
    std::string large_payload;
    large_payload.reserve(1024 * 1024); // 1MB
    for (size_t i = 0; i < 1024 * 1024; ++i) {
        large_payload += static_cast<char>('A' + (i % 26));
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate processing the large payload
    size_t sum = 0;
    for (char c : large_payload) {
        sum += static_cast<unsigned char>(c);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Processed 1MB payload in: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Checksum: " << sum << std::endl;
}

void run_performance_tests() {
    std::cout << "Starting performance benchmarks..." << std::endl;
    
    benchmark_simple_requests();
    std::cout << std::endl;
    
    benchmark_concurrent_requests();
    std::cout << std::endl;
    
    benchmark_large_payloads();
    std::cout << std::endl;
    
    std::cout << "Performance benchmarks completed!" << std::endl;
}

int main() {
    try {
        run_performance_tests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Performance test failed with unknown exception" << std::endl;
        return 1;
    }
}