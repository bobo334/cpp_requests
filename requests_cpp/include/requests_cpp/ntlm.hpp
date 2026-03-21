#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace requests_cpp {

/**
 * 将UTF-8字符串转换为UTF-16LE字节数组
 * @param utf8_str UTF-8编码的字符串
 * @return UTF-16LE字节数组
 */
std::vector<uint8_t> utf8_to_utf16le(const std::string& utf8_str);

/**
 * 将字符串转换为大写
 * @param str 输入字符串
 * @return 大写字符串
 */
std::string to_upper(const std::string& str);

/**
 * 计算NTLM哈希 (MD4 of UTF-16LE password)
 * @param password 明文密码
 * @return 16字节的NTLM哈希
 */
std::vector<uint8_t> calculate_ntlm_hash(const std::string& password);

/**
 * 计算LM哈希 (向后兼容，不安全)
 * @param password 明文密码
 * @return 16字节的LM哈希
 */
std::vector<uint8_t> calculate_lm_hash(const std::string& password);

/**
 * 计算NTLMv2哈希
 * @param username 用户名
 * @param target 目标名称
 * @param ntlm_hash NTLM哈希
 * @param server_challenge 服务器挑战
 * @param client_challenge 客户端挑战
 * @return NTLMv2响应
 */
std::vector<uint8_t> calculate_ntlmv2_hash(
    const std::string& username,
    const std::string& target,
    const std::vector<uint8_t>& ntlm_hash,
    const std::string& server_challenge,
    const std::string& client_challenge);

/**
 * 创建NTLM Type 1消息
 * @param domain 域名
 * @param workstation 工作站名
 * @return Base64编码的NTLM Type 1消息
 */
std::string create_ntlm_type1(const std::string& domain, const std::string& workstation);

/**
 * 创建NTLM Type 3消息
 * @param username 用户名
 * @param domain 域名
 * @param workstation 工作站名
 * @param password 密码
 * @param server_challenge 服务器挑战
 * @param target_info 目标信息
 * @return Base64编码的NTLM Type 3消息
 */
std::string create_ntlm_type3(
    const std::string& username,
    const std::string& domain,
    const std::string& workstation,
    const std::string& password,
    const std::string& server_challenge,
    const std::string& target_info);

/**
 * Base64编码实现
 * @param buf 要编码的字节数组
 * @param bufLen 字节数组长度
 * @return Base64编码的字符串
 */
std::string base64_encode_impl(unsigned char const* buf, unsigned int bufLen);

}  // namespace requests_cpp