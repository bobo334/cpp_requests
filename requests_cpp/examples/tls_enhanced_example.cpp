#include "requests_cpp/tls_enhanced.hpp"
#include <iostream>

int main() {
    std::cout << "=== TLS Enhanced Example ===" << std::endl;

    // Example 1: certificate pinning
    std::cout << "\n--- Certificate Pinning ---" << std::endl;
    try {
        requests_cpp::tls::EnhancedTlsClient client;
        requests_cpp::tls::EnhancedTlsConfig config;

        config.pinning_options.enabled = true;
        config.pinning_options.public_key_hashes = {
            "example_spki_hash_1",
            "example_spki_hash_2"
        };
        config.pinning_options.certificate_hashes = {
            "example_cert_hash_1",
            "example_cert_hash_2"
        };

        std::string error;
        // client.connect("example.com", 443, config, error);

        std::cout << "Certificate pinning config is ready" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Certificate pinning example error: " << e.what() << std::endl;
    }

    // Example 2: CRL and OCSP
    std::cout << "\n--- CRL and OCSP ---" << std::endl;
    try {
        requests_cpp::tls::EnhancedTlsClient client;
        requests_cpp::tls::EnhancedTlsConfig config;

        config.revocation_options.enable_crl_check = true;
        config.revocation_options.enable_ocsp_verification = true;
        config.revocation_options.enable_ocsp_stapling = true;
        config.revocation_options.crl_urls = {
            "http://example.com/crl.pem",
            "http://example.com/crl2.pem"
        };

        std::string error;
        // client.connect("example.com", 443, config, error);

        std::cout << "CRL and OCSP config is ready" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "CRL and OCSP example error: " << e.what() << std::endl;
    }

    // Example 3: TLS session reuse
    std::cout << "\n--- TLS Session Reuse ---" << std::endl;
    try {
        requests_cpp::tls::EnhancedTlsClient client;
        requests_cpp::tls::EnhancedTlsConfig config;

        config.session_options.enable_session_id_reuse = true;
        config.session_options.enable_session_tickets = true;
        config.session_options.session_cache_size = 50;

        std::string error;
        // client.connect("example.com", 443, config, error);

        std::cout << "TLS session reuse config is ready" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "TLS session reuse example error: " << e.what() << std::endl;
    }

    // Example 4: hostname verification
    std::cout << "\n--- Hostname Verification ---" << std::endl;
    try {
        requests_cpp::tls::EnhancedTlsClient client;
        requests_cpp::tls::EnhancedTlsConfig config;

        config.hostname_verification_options.enabled = true;
        config.hostname_verification_options.allowed_hostnames = {
            "example.com",
            "*.example.com",
            "subdomain.example.com"
        };
        config.hostname_verification_options.allow_self_signed = false;

        std::string error;
        // client.connect("example.com", 443, config, error);

        std::cout << "Hostname verification config is ready" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Hostname verification example error: " << e.what() << std::endl;
    }

    // Example 5: combined config
    std::cout << "\n--- Combined Config ---" << std::endl;
    try {
        requests_cpp::tls::EnhancedTlsClient client;
        requests_cpp::tls::EnhancedTlsConfig config;

        config.verify = true;
        config.connect_timeout_ms = 10000;

        config.pinning_options.enabled = true;
        config.pinning_options.public_key_hashes = {"example_spki_hash"};

        config.revocation_options.enable_crl_check = true;
        config.revocation_options.enable_ocsp_verification = true;

        config.session_options.enable_session_id_reuse = true;
        config.session_options.enable_session_tickets = true;

        config.hostname_verification_options.enabled = true;
        config.hostname_verification_options.allowed_hostnames = {"example.com", "*.example.com"};

        std::string error;
        // client.connect("example.com", 443, config, error);

        std::cout << "Combined config is ready" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Combined example error: " << e.what() << std::endl;
    }

    std::cout << "\n=== TLS Enhanced Example Done ===" << std::endl;
    return 0;
}
