#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace requests_cpp {

/**
 * 计算输入字符串的MD5哈希值
 * @param input 输入字符串
 * @return 32字符的十六进制MD5哈希值
 */
std::string md5_hash(const std::string& input);

/**
 * 将字符串转换为大写
 * @param str 输入字符串
 * @return 大写字符串
 */
std::string to_upper(const std::string& str);

/**
 * 去除字符串首尾空白字符
 * @param str 输入字符串
 * @return 去除空白后的字符串
 */
std::string trim(const std::string& str);

}  // namespace requests_cpp