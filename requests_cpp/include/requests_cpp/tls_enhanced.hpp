#pragma once

#include "requests_cpp/tls.hpp"
#include <string>
#include <vector>
#include <memory>

namespace requests_cpp::tls {

/**
 * 证书固定选项
 */
struct CertificatePinningOptions {
    // 公钥哈希固定（SPKI）
    std::vector<std::string> public_key_hashes;
    
    // 证书哈希固定
    std::vector<std::string> certificate_hashes;
    
    // 是否启用证书固定
    bool enabled{false};
};

/**
 * CRL/OCSP选项
 */
struct RevocationOptions {
    // CRL分发点URL列表
    std::vector<std::string> crl_urls;
    
    // 是否启用OCSP装订
    bool enable_ocsp_stapling{false};
    
    // 是否启用OCSP验证
    bool enable_ocsp_verification{false};
    
    // 是否启用CRL验证
    bool enable_crl_check{false};
};

/**
 * TLS会话选项
 */
struct SessionOptions {
    // 是否启用会话ID复用
    bool enable_session_id_reuse{true};
    
    // 是否启用Session Ticket
    bool enable_session_tickets{true};
    
    // 会话缓存大小
    size_t session_cache_size{100};
};

/**
 * 主机名校验选项
 */
struct HostnameVerificationOptions {
    // 是否启用主机名校验
    bool enabled{true};
    
    // 允许的主机名模式（支持通配符）
    std::vector<std::string> allowed_hostnames;
    
    // 是否允许自签名证书
    bool allow_self_signed{false};
};

/**
 * 增强型TLS配置
 */
struct EnhancedTlsConfig : public TlsConfig {
    CertificatePinningOptions pinning_options;
    RevocationOptions revocation_options;
    SessionOptions session_options;
    HostnameVerificationOptions hostname_verification_options;
};

/**
 * 增强型TLS客户端
 * 提供证书固定、CRL/OCSP、会话复用和主机名校验功能
 */
class EnhancedTlsClient {
public:
    EnhancedTlsClient();
    ~EnhancedTlsClient();

    EnhancedTlsClient(const EnhancedTlsClient&) = delete;
    EnhancedTlsClient& operator=(const EnhancedTlsClient&) = delete;
    EnhancedTlsClient(EnhancedTlsClient&&) noexcept;
    EnhancedTlsClient& operator=(EnhancedTlsClient&&) noexcept;

    /**
     * 连接到TLS服务器
     */
    bool connect(const std::string& host, std::uint16_t port, const EnhancedTlsConfig& config, std::string& error);

    /**
     * 发送数据
     */
    std::size_t send(const std::string& data);

    /**
     * 接收数据
     */
    std::string recv_some(std::size_t max_bytes = 4096);

    /**
     * 关闭连接
     */
    void close();

    /**
     * 检查连接是否打开
     */
    bool is_open() const noexcept;

    /**
     * 获取证书公钥哈希（用于证书固定）
     */
    std::string get_peer_public_key_hash() const;

    /**
     * 获取证书哈希（用于证书固定）
     */
    std::string get_peer_certificate_hash() const;

private:
    // 这些校验函数是 connect() 的内部步骤，放在 private 避免外部误用。
    bool validate_certificate_pinning(const std::string& host, std::string& error);
    bool validate_hostname(const std::string& hostname, std::string& error);
    bool validate_revocation_status(std::string& error);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace requests_cpp::tls