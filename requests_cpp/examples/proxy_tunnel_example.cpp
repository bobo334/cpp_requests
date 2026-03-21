#include "requests_cpp/proxy_tunnel.hpp"
#include "requests_cpp/proxy.hpp"
#include <iostream>
#include <memory>

int main() {
    std::cout << "=== Proxy Tunnel Example ===" << std::endl;

    // Example 1: CONNECT tunnel
    std::cout << "\n--- CONNECT Tunnel ---" << std::endl;
    try {
        auto tunnel = requests_cpp::ConnectTunnel::establish(
            "proxy.example.com",
            8080,
            "www.google.com",
            443
        );

        if (tunnel) {
            std::cout << "CONNECT tunnel is ready" << std::endl;
        } else {
            std::cout << "CONNECT tunnel failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "CONNECT example error: " << e.what() << std::endl;
    }

    // Example 2: SOCKS5
    std::cout << "\n--- SOCKS5 ---" << std::endl;
    try {
        auto socks5_conn = requests_cpp::Socks5Client::connect(
            "socks5.proxy.com",
            1080,
            "www.example.com",
            80,
            "username",
            "password"
        );

        if (socks5_conn) {
            std::cout << "SOCKS5 connection is ready" << std::endl;
        } else {
            std::cout << "SOCKS5 connection failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "SOCKS5 example error: " << e.what() << std::endl;
    }

    // Example 3: SOCKS4
    std::cout << "\n--- SOCKS4 ---" << std::endl;
    try {
        auto socks4_conn = requests_cpp::Socks4Client::connect(
            "socks4.proxy.com",
            1080,
            "www.example.com",
            80,
            "userid"
        );

        if (socks4_conn) {
            std::cout << "SOCKS4 connection is ready" << std::endl;
        } else {
            std::cout << "SOCKS4 connection failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "SOCKS4 example error: " << e.what() << std::endl;
    }

    // Example 4: factory API
    std::cout << "\n--- Factory API ---" << std::endl;
    try {
        auto http_tunnel = requests_cpp::ProxyTunnelFactory::create_tunnel(
            "http://proxy.example.com:8080",
            "secure.example.com",
            443
        );

        if (http_tunnel) {
            std::cout << "HTTP proxy tunnel is ready" << std::endl;
        } else {
            std::cout << "HTTP proxy tunnel failed" << std::endl;
        }

        auto socks5_tunnel = requests_cpp::ProxyTunnelFactory::create_tunnel(
            "socks5://user:pass@socks5.example.com:1080",
            "target.example.com",
            80,
            "user",
            "pass"
        );

        if (socks5_tunnel) {
            std::cout << "SOCKS5 tunnel is ready" << std::endl;
        } else {
            std::cout << "SOCKS5 tunnel failed" << std::endl;
        }

        auto socks4_tunnel = requests_cpp::ProxyTunnelFactory::create_tunnel(
            "socks4://socks4.example.com:1080",
            "target.example.com",
            80
        );

        if (socks4_tunnel) {
            std::cout << "SOCKS4 tunnel is ready" << std::endl;
        } else {
            std::cout << "SOCKS4 tunnel failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Factory example error: " << e.what() << std::endl;
    }

    std::cout << "\n=== Proxy Tunnel Example Done ===" << std::endl;
    return 0;
}
