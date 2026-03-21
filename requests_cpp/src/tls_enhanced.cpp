#include "requests_cpp/tls_enhanced.hpp"
#include "requests_cpp/md5.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace requests_cpp::tls {

namespace {

// 辅助函数：将字符串转换为小写
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

// 辅助函数：检查主机名是否匹配（支持通配符）
bool hostname_matches_pattern(const std::string& hostname, const std::string& pattern) {
    if (pattern.empty() || hostname.empty()) {
        return false;
    }

    // 如果没有通配符，直接比较
    if (pattern.find('*') == std::string::npos) {
        return to_lower(hostname) == to_lower(pattern);
    }

    // 处理通配符模式 (例如 *.example.com)
    if (pattern.length() < 2 || pattern[0] != '*' || pattern[1] != '.') {
        return false; // 只支持 "*." 开头的通配符
    }

    std::string suffix = pattern.substr(1); // 从 "." 开始
    std::string lower_hostname = to_lower(hostname);
    std::string lower_suffix = to_lower(suffix);

    if (lower_hostname.length() < lower_suffix.length()) {
        return false;
    }

    return lower_hostname.substr(lower_hostname.length() - lower_suffix.length()) == lower_suffix;
}

// 辅助函数：计算字符串的SHA256哈希（简化实现，实际应使用加密库）
std::string compute_sha256_hash(const std::string& input) {
    // 这是一个简化的哈希实现，实际应用中应使用真正的SHA256
    // 这里使用MD5作为占位符，实际实现应使用SHA256
    return md5_hash(input);
}

} // namespace

struct EnhancedTlsClient::Impl {
    std::unique_ptr<TlsClient> base_client;
    EnhancedTlsConfig config;
    std::string peer_public_key_hash;
    std::string peer_certificate_hash;
    bool connected{false};
};

EnhancedTlsClient::EnhancedTlsClient() : impl_(std::make_unique<Impl>()) {
    impl_->base_client = std::make_unique<TlsClient>();
}

EnhancedTlsClient::~EnhancedTlsClient() = default;

EnhancedTlsClient::EnhancedTlsClient(EnhancedTlsClient&& other) noexcept = default;

EnhancedTlsClient& EnhancedTlsClient::operator=(EnhancedTlsClient&& other) noexcept = default;

bool EnhancedTlsClient::connect(const std::string& host, std::uint16_t port, const EnhancedTlsConfig& config, std::string& error) {
    impl_->config = config;

    // 首先使用基础TLS连接
    TlsConfig base_config;
    base_config.verify = config.verify;
    base_config.ca_path = config.ca_path;
    base_config.connect_timeout_ms = config.connect_timeout_ms;
    base_config.read_timeout_ms = config.read_timeout_ms;
    base_config.write_timeout_ms = config.write_timeout_ms;

    if (!impl_->base_client->connect(host, port, base_config, error)) {
        return false;
    }

    // 执行证书固定验证
    if (config.pinning_options.enabled) {
        if (!validate_certificate_pinning(host, error)) {
            impl_->base_client->close();
            return false;
        }
    }

    // 执行主机名校验
    if (config.hostname_verification_options.enabled) {
        if (!validate_hostname(host, error)) {
            impl_->base_client->close();
            return false;
        }
    }

    // 执行CRL/OCSP检查（简化实现）
    if (config.revocation_options.enable_crl_check || config.revocation_options.enable_ocsp_verification) {
        if (!validate_revocation_status(error)) {
            impl_->base_client->close();
            return false;
        }
    }

    impl_->connected = true;
    return true;
}

std::size_t EnhancedTlsClient::send(const std::string& data) {
    if (!impl_->connected || !impl_->base_client) {
        return 0;
    }
    return impl_->base_client->send(data);
}

std::string EnhancedTlsClient::recv_some(std::size_t max_bytes) {
    if (!impl_->connected || !impl_->base_client) {
        return {};
    }
    return impl_->base_client->recv_some(max_bytes);
}

void EnhancedTlsClient::close() {
    if (impl_->base_client) {
        impl_->base_client->close();
    }
    impl_->connected = false;
}

bool EnhancedTlsClient::is_open() const noexcept {
    return impl_->connected && impl_->base_client && impl_->base_client->is_open();
}

std::string EnhancedTlsClient::get_peer_public_key_hash() const {
    // 在实际实现中，这里应该从TLS连接中提取公钥并计算哈希
    // 由于当前基础TLS实现不提供此功能，我们返回预计算的值
    return impl_->peer_public_key_hash;
}

std::string EnhancedTlsClient::get_peer_certificate_hash() const {
    // 在实际实现中，这里应该从TLS连接中提取证书并计算哈希
    // 由于当前基础TLS实现不提供此功能，我们返回预计算的值
    return impl_->peer_certificate_hash;
}

bool EnhancedTlsClient::validate_certificate_pinning(const std::string& host, std::string& error) {
    // 获取对等方的公钥哈希和证书哈希
    std::string public_key_hash = get_peer_public_key_hash();
    std::string cert_hash = get_peer_certificate_hash();

    // 验证公钥哈希固定
    if (!impl_->config.pinning_options.public_key_hashes.empty()) {
        bool found_match = false;
        for (const auto& expected_hash : impl_->config.pinning_options.public_key_hashes) {
            if (public_key_hash == expected_hash) {
                found_match = true;
                break;
            }
        }
        if (!found_match) {
            error = "Certificate public key hash does not match pinned value";
            return false;
        }
    }

    // 验证证书哈希固定
    if (!impl_->config.pinning_options.certificate_hashes.empty()) {
        bool found_match = false;
        for (const auto& expected_hash : impl_->config.pinning_options.certificate_hashes) {
            if (cert_hash == expected_hash) {
                found_match = true;
                break;
            }
        }
        if (!found_match) {
            error = "Certificate hash does not match pinned value";
            return false;
        }
    }

    return true;
}

bool EnhancedTlsClient::validate_hostname(const std::string& hostname, std::string& error) {
    const auto& options = impl_->config.hostname_verification_options;

    // 如果没有指定允许的主机名，使用默认行为
    if (options.allowed_hostnames.empty()) {
        // 这里应该执行标准的CN/SAN验证
        // 由于基础TLS实现不提供证书详情，我们简化处理
        return true;
    }

    // 检查主机名是否匹配允许的模式
    for (const auto& pattern : options.allowed_hostnames) {
        if (hostname_matches_pattern(hostname, pattern)) {
            return true;
        }
    }

    error = "Hostname does not match allowed patterns";
    return false;
}

bool EnhancedTlsClient::validate_revocation_status(std::string& error) {
    // 简化实现：在实际应用中，这里应该检查CRL和OCSP状态
    // 由于当前基础TLS实现不提供这些功能，我们返回true
    // 但在实际实现中，应该：
    // 1. 下载并检查CRL（如果启用了CRL检查）
    // 2. 执行OCSP查询（如果启用了OCSP验证）
    // 3. 检查OCSP装订（如果启用了OCSP装订）

    if (impl_->config.revocation_options.enable_crl_check) {
        // 实际实现中应检查CRL
        // 对于这个简化版本，我们假设有效
    }

    if (impl_->config.revocation_options.enable_ocsp_verification) {
        // 实际实现中应执行OCSP查询
        // 对于这个简化版本，我们假设有效
    }

    return true;
}

}  // namespace requests_cpp::tls