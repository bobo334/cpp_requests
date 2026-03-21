# Requests C++17 移植方案

## 项目概述

将 Python Requests 库移植为 C++17 原生实现，提供简洁的 HTTP 客户端 API。

### 目标与约束
- **目标**：实现完整的 HTTP/1.1 和 HTTP/2 客户端功能，保持 Requests 风格 API
- **平台**：Windows / Linux / macOS
- **构建**：CMake，静态库
- **TLS**：TinyTls（GPLv3），全平台统一实现
- **标准**：C++17

---

## 实现进度

### ✅ HTTP/1.1 核心
| 功能 | 状态 | 说明 |
|------|------|------|
| 响应解析 | ✅ | 状态行/headers/body，Content-Length/chunked/close |
| Chunked Trailers | ✅ | 合并到响应 headers |
| Content-Encoding | ✅ | gzip/deflate(miniz)/brotli，多重编码逆序解码 |
| 请求序列化 | ✅ | 请求行/headers/body |
| URL 解析 | ✅ | scheme/host/port/path/query，IPv6 支持 |
| 连接池 | ✅ | 按主机复用，空闲超时，Keep-Alive |
| 重定向 | ✅ | 301/302/303/307/308，方法切换 |
| 重试 | ✅ | 基础重试循环 |

### ✅ HTTP/2 支持
| 功能 | 状态 | 说明 |
|------|------|------|
| 协议握手 | ✅ | preface + SETTINGS 帧 |
| nghttp2 集成 | ✅ | 完整源码编译 |
| HPACK | ✅ | 头部压缩（nghttp2 内置） |
| 流量控制 | ✅ | WINDOW_UPDATE，可配置窗口大小 |
| 并发调度 | ✅ | max_concurrent_streams 配置 |
| 错误恢复 | ✅ | RST_STREAM/GOAWAY 处理 |
| 自动回退 | ✅ | HTTP/2 失败自动切换 HTTP/1.1 |
| Session 开关 | ✅ | enable_http2(bool) |

### ✅ TLS/HTTPS
| 功能 | 状态 | 说明 |
|------|------|------|
| 统一实现 | ✅ | 全平台共享代码，条件编译处理差异 |
| Windows | ✅ | TinyTls + WinSock2 |
| Linux | ✅ | TinyTls + POSIX Socket |
| macOS | ✅ | TinyTls + POSIX Socket |
| 证书校验 | ✅ | verify=true/false |
| 自定义 CA | ✅ | PEM/DER 格式 |
| SNI | ✅ | Server Name Indication |
| 根证书内置 | ✅ | Mozilla NSS 144 个证书 |
| 系统证书 | ✅ | Windows Store/Linux/macOS Keychain |
| TLS 连接池 | ✅ | 复用连接 |
| TLS 1.3 | ✅ | AES_128_GCM_SHA256, CHACHA20_POLY1305_SHA256 |
| TLS 1.2 | ✅ | ECDHE_RSA/ECDSA with AES_GCM/CHACHA20 |
| 超时配置 | ✅ | connect/read/write timeout |
| 证书回调 | ✅ | CB_CLIENT_CIPHER/CB_SERVER_NAME/CB_CERTIFICATE_ALERT |
| 证书 Pinning | ✅ | 公钥指纹/证书指纹/SPKI hash |
| CRL/OCSP | ✅ | CRL 加载验证、OCSP stapling |
| TLS 会话复用 | ✅ | Session ID 复用、Session Ticket |
| 主机名校验 | ✅ | CN/SAN 验证、通配符证书支持 |

### ✅ 代理支持
| 功能 | 状态 | 说明 |
|------|------|------|
| HTTP 代理 | ✅ | ProxyConfig 配置 |
| HTTPS 代理 | ✅ | 独立配置项 |
| SOCKS4 代理 | ✅ | socks4_proxy 配置 |
| SOCKS5 代理 | ✅ | socks5_proxy 配置 |
| 环境变量 | ✅ | HTTP_PROXY/HTTPS_PROXY/SOCKS_PROXY |
| NO_PROXY | ✅ | 主机匹配、通配符、CIDR |
| ProxyManager | ✅ | URL 代理选择、绕过检测 |
| CONNECT 隧道 | ✅ | HTTP/HTTPS 代理隧道建立 |
| SOCKS4 协议 | ✅ | 完整握手实现 |
| SOCKS5 协议 | ✅ | 握手 + 用户名密码认证 |
| ProxyTunnelFactory | ✅ | 根据代理 URL 自动选择协议 |

### ✅ 认证实现
| 功能 | 状态 | 说明 |
|------|------|------|
| Auth 基类 | ✅ | get_auth_header() 接口 |
| Basic Auth | ✅ | Base64 编码 |
| Bearer Token | ✅ | Authorization: Bearer |
| Digest Auth | ✅ | 完整 MD5 实现、challenge 解析、cnonce 生成 |
| NTLM Auth | ✅ | Type 1/Type 3 消息、LM/NTLMv2 哈希、RC4 加密 |
| MD5 模块 | ✅ | 完整 RFC 1321 实现 |
| NTLM 模块 | ✅ | UTF-16LE 转换、哈希计算、消息生成 |

### ✅ Cookie 管理
| 功能 | 状态 | 说明 |
|------|------|------|
| Cookie 结构 | ✅ | name/value/domain/path/expires/secure/httponly/samesite |
| CookieJar | ✅ | 添加/获取/清理 |
| Set-Cookie 解析 | ✅ | 属性解析、域名规范化 |
| 域名匹配 | ✅ | RFC 6265 子域名匹配 |
| 过期处理 | ✅ | is_expired()/clear_expired() |
| 文件持久化 | ✅ | save_to_file()/load_from_file() |
| Mozilla 格式 | ✅ | save_as_mozilla_format()/load_from_mozilla_format() |

### ✅ 重试策略
| 功能 | 状态 | 说明 |
|------|------|------|
| RetryConfig | ✅ | total_retries/backoff_factor/max_backoff |
| 指数退避 | ✅ | 2^n * backoff_factor |
| Jitter | ✅ | 随机抖动避免惊群 |
| 幂等方法检测 | ✅ | GET/HEAD/PUT/DELETE/OPTIONS/TRACE |
| 状态码重试 | ✅ | 可配置 status_codes_to_retry |

### ✅ 高级功能
| 功能 | 状态 | 说明 |
|------|------|------|
| 流式读取 | ✅ | 按块读取回调、延迟加载、大文件下载 |
| Hooks 扩展 | ✅ | 请求前/响应后/错误处理回调 |
| 日志与调试 | ✅ | DEBUG/INFO/WARN/ERROR 级别、自定义回调 |
| AdvancedSession | ✅ | 集成流式读取、Hooks、日志功能 |

### ✅ API 与架构
| 功能 | 状态 | 说明 |
|------|------|------|
| 便捷函数 | ✅ | get/post/request |
| Session 类 | ✅ | 默认 headers/timeout/max_redirects |
| Request/Response | ✅ | 完整数据结构 |
| Adapter 抽象 | ✅ | HTTP/1.1 和 HTTP/2 适配器 |
| 异常层次 | ✅ | 10 种异常类型 |
| 错误码 | ✅ | Network/Timeout/TLS/HTTP2/IO/Protocol |

### ✅ 构建与基础设施
| 功能 | 状态 | 说明 |
|------|------|------|
| CMake | ✅ | 静态库/动态库构建 |
| BUILD_SHARED_LIBS | ✅ | 动态库选项 |
| GNUInstallDirs | ✅ | 安装目录支持 |
| 第三方库 | ✅ | miniz/brotli/nghttp2/TinyTls |
| CI/CD | ✅ | GitHub Actions (Win/Linux/macOS) |
| 示例程序 | ✅ | 8 个示例 |

### ✅ 包管理
| 功能 | 状态 | 说明 |
|------|------|------|
| vcpkg | ✅ | vcpkg.json 配置 |
| Conan | ✅ | conanfile.py 配置 |
| 依赖管理 | ✅ | openssl/zlib/brotli/nghttp2 |

### ✅ 测试覆盖
| 功能 | 状态 | 说明 |
|------|------|------|
| 单元测试 | ✅ | unit_tests.cpp |
| 性能测试 | ✅ | performance_tests.cpp |
| 代理隧道测试 | ✅ | proxy_tunnel_test.cpp |

### ✅ 文档
| 功能 | 状态 | 说明 |
|------|------|------|
| API 文档 | ✅ | api_documentation.md |
| 用户手册 | ✅ | user_manual.md |
| Sphinx 文档 | ✅ | 完整文档结构 |

---

## 项目完成状态

**🎉 所有核心功能和基础设施已完成！**

---

## 文件结构

```
requests_cpp/
├── include/requests_cpp/
│   ├── adapter.hpp           ✅
│   ├── api.hpp               ✅
│   ├── auth.hpp              ✅ (NTLM 新增)
│   ├── connection_pool.hpp   ✅
│   ├── cookie.hpp            ✅ (持久化新增)
│   ├── exceptions.hpp        ✅
│   ├── export.hpp            ✅
│   ├── http1.hpp             ✅
│   ├── http2.hpp             ✅
│   ├── http2_smoke.hpp       ✅
│   ├── md5.hpp               ✅ 新增（完整 MD5 实现）
│   ├── ntlm.hpp              ✅ 新增（NTLM 认证模块）
│   ├── proxy.hpp             ✅ (SOCKS4/5 新增)
│   ├── proxy_tunnel.hpp      ✅ 新增（CONNECT/SOCKS 隧道）
│   ├── request.hpp           ✅
│   ├── response.hpp          ✅
│   ├── retry.hpp             ✅
│   ├── session.hpp           ✅ (NTLM 新增)
│   ├── streaming.hpp         ✅ 新增（流式读取、Hooks、日志）
│   ├── system_certs.hpp      ✅
│   ├── tls.hpp               ✅
│   ├── tls_enhanced.hpp      ✅ 新增（TLS 增强功能）
│   ├── types.hpp             ✅
│   └── url.hpp               ✅
├── src/
│   ├── adapter.cpp           ✅
│   ├── api.cpp               ✅
│   ├── auth.cpp              ✅
│   ├── connection_pool.cpp   ✅
│   ├── cookie.cpp            ✅
│   ├── exceptions.cpp        ✅
│   ├── http1.cpp             ✅
│   ├── http1_adapter.cpp     ✅
│   ├── http2_adapter.cpp     ✅
│   ├── http2_smoke.cpp       ✅
│   ├── md5.cpp               ✅ 新增（完整 RFC 1321 MD5 实现）
│   ├── ntlm.cpp              ✅ 新增（NTLM 认证完整实现）
│   ├── proxy.cpp             ✅
│   ├── proxy_tunnel.cpp      ✅ 新增（CONNECT/SOCKS 隧道实现）
│   ├── request.cpp           ✅
│   ├── response.cpp          ✅
│   ├── retry.cpp             ✅
│   ├── root_certs.hpp        ✅
│   ├── root_certs.pem        ✅
│   ├── session.cpp           ✅
│   ├── streaming.cpp         ✅ 新增（流式读取、Hooks、日志实现）
│   ├── system_certs.cpp      ✅
│   ├── tls_connection_pool.cpp ✅
│   ├── tls_enhanced.cpp       ✅ 新增（TLS 增强功能实现）
│   ├── tls_linux.cpp         ✅ 统一实现
│   ├── tls_macos.cpp         ✅ 统一实现
│   ├── tls_stub.cpp          ⚠️ 空实现（不支持平台）
│   ├── tls_win.cpp           ✅ 统一实现
│   └── url.cpp               ✅
├── third_party/
│   ├── TinyTls/              ✅
│   ├── brotli/               ✅
│   ├── miniz/                ✅
│   └── nghttp2/              ✅
├── examples/
│   ├── advanced_auth_proxy_example.cpp  ✅ 新增
│   ├── advanced_features_example.cpp    ✅
│   ├── decode_chain_smoke.cpp          ✅
│   ├── http1_chunked_trailers_decode.cpp ✅
│   ├── http2_smoke.cpp                 ✅
│   ├── proxy_tunnel_example.cpp        ✅ 新增（代理隧道示例）
│   └── tls_enhanced_example.cpp        ✅ 新增（TLS 增强功能示例）
├── tests/
│   ├── unit_tests.cpp          ✅ 新增（单元测试）
│   ├── performance_tests.cpp   ✅ 新增（性能测试）
│   └── proxy_tunnel_test.cpp   ✅ 新增（代理隧道测试）
├── docs/
│   ├── api_documentation.md    ✅ 新增（API 文档）
│   ├── user_manual.md          ✅ 新增（用户手册）
│   └── ...                     ✅ Sphinx 文档结构
├── vcpkg.json                  ✅ 新增（vcpkg 配置）
├── conanfile.py                ✅ 新增（Conan 配置）
└── CMakeLists.txt              ✅
```

---

## API 参考

### 便捷函数
```cpp
namespace requests_cpp {
    Response get(const std::string& url);
    Response post(const std::string& url);
    Response post(const std::string& url, const std::string& body);
    Response request(const std::string& method, const std::string& url);
}
```

### Session 类
```cpp
class Session {
public:
    Session();
    
    Response get(const std::string& url);
    Response post(const std::string& url);
    Response post(const std::string& url, const std::string& body);
    Response request(const std::string& method, const std::string& url);
    Response request(const std::string& method, const std::string& url, 
                     const std::string& body, const Timeout& timeout, int retries = 0);
    
    void enable_http2(bool enable);
    void set_connection_pool_config(http1::ConnectionPoolConfig config);
    void mount(std::shared_ptr<Adapter> adapter);
    
    // Proxy
    void set_proxy_config(const ProxyConfig& config);
    void set_proxy_from_environment();
    
    // Auth
    void set_auth(std::shared_ptr<Auth> auth);
    void set_basic_auth(const std::string& username, const std::string& password);
    void set_bearer_token_auth(const std::string& token);
    void set_digest_auth(const std::string& username, const std::string& password);
    void set_ntlm_auth(const std::string& username, const std::string& password, 
                       const std::string& domain = "", const std::string& workstation = "");
    
    // Cookie
    void set_cookies(std::shared_ptr<CookieJar> cookie_jar);
    CookieJar& cookies();
    
    // Retry
    void set_retry_config(const RetryConfig& config);
    void set_max_retries(int retries);
};
```

### 配置结构体
```cpp
struct Timeout {
    long connect_ms{0};
    long read_ms{0};
};

struct ConnectionPoolConfig {
    std::size_t max_connections{64};
    std::size_t max_connections_per_host{8};
    int idle_timeout_ms{30000};
    int health_check_interval_ms{5000};
};

struct Http2Config {
    std::size_t max_connections{64};
    std::size_t max_connections_per_host{8};
    std::uint32_t max_concurrent_streams{128};
    std::uint32_t initial_stream_window_size{65535};
    std::uint32_t initial_connection_window_size{65535};
    std::uint32_t header_table_size{4096};
    int idle_timeout_ms{30000};
};

struct TlsConfig {
    bool verify{true};
    std::string ca_path{};
    long connect_timeout_ms{0};
    long read_timeout_ms{0};
    long write_timeout_ms{0};
};

struct ProxyConfig {
    std::string http_proxy;
    std::string https_proxy;
    std::string socks_proxy;
    std::string socks4_proxy;
    std::string socks5_proxy;
    std::vector<std::string> no_proxy_hosts;
    static ProxyConfig from_environment();
};

struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::chrono::system_clock::time_point expires;
    bool secure{false};
    bool httponly{false};
    std::string samesite;
};

struct RetryConfig {
    int total_retries{3};
    int connect_retries{3};
    int read_retries{3};
    int status_retries{3};
    std::vector<int> status_codes_to_retry{413, 429, 503};
    std::chrono::milliseconds backoff_factor{1000};
    std::chrono::milliseconds max_backoff{10000};
    bool respect_retry_after_header{true};
};
```

### 代理隧道类
```cpp
class ConnectTunnel {
public:
    static std::unique_ptr<SocketConnection> establish(
        const std::string& proxy_host, int proxy_port,
        const std::string& dest_host, int dest_port);
};

class Socks4Client {
public:
    static std::unique_ptr<SocketConnection> connect(
        const std::string& proxy_host, int proxy_port,
        const std::string& dest_host, int dest_port,
        const std::string& user_id = "");
};

class Socks5Client {
public:
    static std::unique_ptr<SocketConnection> connect(
        const std::string& proxy_host, int proxy_port,
        const std::string& dest_host, int dest_port,
        const std::string& username = "",
        const std::string& password = "");
};

class ProxyTunnelFactory {
public:
    static std::unique_ptr<SocketConnection> create_tunnel(
        const std::string& proxy_url,
        const std::string& dest_host, int dest_port,
        const std::string& username = "",
        const std::string& password = "");
};
```

### 认证类
```cpp
class Auth {
public:
    virtual ~Auth() = default;
    virtual std::string get_auth_header() const = 0;
};

class BasicAuth : public Auth { ... };
class BearerTokenAuth : public Auth { ... };
class DigestAuth : public Auth { ... };
class NtlmAuth : public Auth {
public:
    NtlmAuth(const std::string& username, const std::string& password, 
             const std::string& domain = "", const std::string& workstation = "");
    void process_challenge(const std::string& challenge);
    // Type 1/Type 3 消息生成
};
```

### MD5 模块
```cpp
namespace requests_cpp {
    std::string md5_hash(const std::string& input);
    std::string to_upper(const std::string& str);
    std::string trim(const std::string& str);
}
```

### NTLM 模块
```cpp
namespace requests_cpp {
    std::vector<uint8_t> utf8_to_utf16le(const std::string& utf8_str);
    std::vector<uint8_t> calculate_ntlm_hash(const std::string& password);
    std::vector<uint8_t> calculate_lm_hash(const std::string& password);
    std::vector<uint8_t> calculate_ntlmv2_hash(
        const std::string& username,
        const std::string& target,
        const std::vector<uint8_t>& ntlm_hash,
        const std::string& server_challenge,
        const std::string& client_challenge);
    std::string create_ntlm_type1(const std::string& domain, const std::string& workstation);
    std::string create_ntlm_type3(
        const std::string& username,
        const std::string& domain,
        const std::string& workstation,
        const std::string& password,
        const std::string& server_challenge,
        const std::string& target_info);
}
```

### CookieJar 持久化
```cpp
class CookieJar {
public:
    // 基础操作
    void add_cookie(const Cookie& cookie);
    std::vector<Cookie> get_cookies(const std::string& domain, const std::string& path) const;
    void clear_expired();
    
    // 持久化
    bool save_to_file(const std::string& filepath) const;
    bool load_from_file(const std::string& filepath);
    bool save_as_mozilla_format(const std::string& filepath) const;
    bool load_from_mozilla_format(const std::string& filepath);
};
```

### TlsClient 类
```cpp
class TlsClient {
public:
    TlsClient();
    ~TlsClient();
    
    bool connect(const std::string& host, std::uint16_t port, 
                 const TlsConfig& config, std::string& error);
    bool add_root_certificate_pem(const std::string& pem, std::string& error);
    bool add_root_certificate_file(const std::string& path, std::string& error);
    std::size_t send(const std::string& data);
    std::string recv_some(std::size_t max_bytes);
    void close();
    bool is_open() const noexcept;
};
```

### TLS 增强功能
```cpp
namespace requests_cpp::tls {

struct CertificatePinningOptions {
    std::vector<std::string> public_key_hashes;    // SPKI 公钥指纹
    std::vector<std::string> certificate_hashes;   // 证书指纹
    bool enabled{false};
};

struct RevocationOptions {
    std::vector<std::string> crl_urls;             // CRL 分发点
    bool enable_ocsp_stapling{false};              // OCSP 装订
    bool enable_ocsp_verification{false};          // OCSP 验证
    bool enable_crl_check{false};                  // CRL 检查
};

struct SessionOptions {
    bool enable_session_id_reuse{true};            // Session ID 复用
    bool enable_session_tickets{true};             // Session Ticket
    size_t session_cache_size{100};                // 缓存大小
};

struct HostnameVerificationOptions {
    bool enabled{true};                            // 启用主机名校验
    std::vector<std::string> allowed_hostnames;    // 允许的主机名（支持通配符）
    bool allow_self_signed{false};                 // 允许自签名证书
};

struct EnhancedTlsConfig : public TlsConfig {
    CertificatePinningOptions pinning_options;
    RevocationOptions revocation_options;
    SessionOptions session_options;
    HostnameVerificationOptions hostname_verification_options;
};

class EnhancedTlsClient {
public:
    EnhancedTlsClient();
    bool connect(const std::string& host, std::uint16_t port, 
                 const EnhancedTlsConfig& config, std::string& error);
    std::size_t send(const std::string& data);
    std::string recv_some(std::size_t max_bytes = 4096);
    void close();
    bool is_open() const noexcept;
    std::string get_peer_public_key_hash() const;
    std::string get_peer_certificate_hash() const;
};

}
```

### 异常类型
```
RequestException (基类)
├── ConnectionError      // 连接失败、DNS 失败
├── TimeoutError         // 连接/读取超时
├── SSLError            // TLS 错误
├── IOError             // 读写错误
├── ProtocolError       // 协议错误
├── HTTP2Error          // HTTP/2 错误
├── InvalidSchemaError  // 不支持的 scheme
├── InvalidURLError     // URL 解析失败
└── HTTPError           // HTTP 状态码错误
```

### 高级功能
```cpp
namespace requests_cpp {

// 流式读取回调
using StreamChunkCallback = std::function<bool(const std::string& chunk)>;

// Hooks 回调
using PreRequestHook = std::function<void(const std::string& url, const std::string& method)>;
using PostResponseHook = std::function<void(int status_code, const std::string& url)>;
using ErrorHook = std::function<void(const std::string& error, const std::string& url)>;

// 日志级别
enum class LogLevel { DEBUG, INFO, WARN, ERROR, NONE };
using LogCallback = std::function<void(LogLevel level, const std::string& message)>;

// 流式响应处理器
class StreamingResponse {
public:
    void set_chunk_callback(StreamChunkCallback callback);
    void set_lazy_load(bool lazy_load);
    void set_chunk_size(size_t chunk_size);
    bool stream(const std::string& url, const std::string& method = "GET", 
               const std::string& body = "");
    long long get_total_size() const;
    long long get_downloaded_size() const;
};

// Hooks 管理器
class HooksManager {
public:
    void add_pre_request_hook(PreRequestHook hook);
    void add_post_response_hook(PostResponseHook hook);
    void add_error_hook(ErrorHook hook);
    void execute_pre_request_hooks(const std::string& url, const std::string& method);
    void execute_post_response_hooks(int status_code, const std::string& url);
    void execute_error_hooks(const std::string& error, const std::string& url);
};

// 日志管理器
class LogManager {
public:
    void set_log_level(LogLevel level);
    void set_log_callback(LogCallback callback);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
};

// 高级会话类（集成所有功能）
class AdvancedSession {
public:
    AdvancedSession();
    void set_stream_chunk_callback(StreamChunkCallback callback);
    void add_pre_request_hook(PreRequestHook hook);
    void add_post_response_hook(PostResponseHook hook);
    void add_error_hook(ErrorHook hook);
    void set_log_level(LogLevel level);
    void set_log_callback(LogCallback callback);
    std::string get(const std::string& url, bool stream_chunks = false);
    std::string post(const std::string& url, const std::string& body = "", bool stream_chunks = false);
};

}
```

---

## TLS 实现细节

### 统一实现架构
TLS 实现已统一为单一代码库，使用条件编译处理平台差异：

| 平台 | Socket API | WSA 初始化 |
|------|-----------|-----------|
| Windows | WinSock2 (SOCKET) | WsaGuard |
| Linux | POSIX Socket (int) | 无需 |
| macOS | POSIX Socket (int) | 无需 |

### 核心组件
- **TinyTlsClientSocket**：跨平台 TCP Socket 封装
- **CallbackContext**：TLS 回调上下文（SNI、证书验证）
- **tls_callback**：TinyTls 回调处理

### 密码套件
```cpp
TLS_AES_128_GCM_SHA256           // TLS 1.3
TLS_CHACHA20_POLY1305_SHA256     // TLS 1.3
TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
```

### 证书加载流程
1. 检查 `ca_path` 是否配置
2. 配置则加载自定义 CA 文件
3. 否则加载内置 Mozilla NSS 根证书
4. 支持 PEM bundle 和 DER 格式

---

## 第三方依赖

| 库 | 许可证 | 用途 |
|---|--------|------|
| TinyTls | GPLv3 | TLS 1.2/1.3 实现 |
| nghttp2 | MIT | HTTP/2 协议 |
| miniz | MIT | gzip/deflate 解压 |
| brotli | MIT | Brotli 解压 |

---

## 构建说明

```bash
# 配置
cmake -S requests_cpp -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release --parallel

# 运行示例
./build/Release/advanced_auth_proxy_example.exe
```

### CMake 选项
- `REQUESTS_CPP_BUILD_EXAMPLES`：构建示例程序（默认 ON）
- `BUILD_SHARED_LIBS`：构建动态库（默认 OFF）

---

## 包管理配置

### vcpkg
```json
{
  "name": "requests-cpp",
  "version-semver": "0.1.0",
  "description": "A modern C++ HTTP library inspired by Python's requests",
  "dependencies": ["openssl", "zlib", "brotli", "nghttp2"],
  "features": {
    "http2": { "dependencies": ["nghttp2"] },
    "ssl": { "dependencies": ["openssl"] }
  }
}
```

### Conan
```python
class RequestsCppConan(ConanFile):
    name = "requests-cpp"
    version = "0.1.0"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_ssl": [True, False],
        "with_http2": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_ssl": True,
        "with_http2": True,
    }
```

---

## 项目统计

| 类别 | 数量 |
|------|------|
| 头文件 | 24 个 |
| 源文件 | 28 个 |
| 示例程序 | 8 个 |
| 测试文件 | 3 个 |
| 文档文件 | 20+ 个 |
| 第三方库 | 4 个 |
| 支持平台 | 3 个 (Windows/Linux/macOS) |

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 0.1.0 | 2024 | 初始版本，完整功能实现 |
