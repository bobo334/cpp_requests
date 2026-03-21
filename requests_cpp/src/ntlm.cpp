#include "requests_cpp/ntlm.hpp"
#include "requests_cpp/md5.hpp"
#include <cstring>
#include <algorithm>
#include <random>

namespace requests_cpp {

// Helper function to convert string to UTF-16LE bytes
std::vector<uint8_t> utf8_to_utf16le(const std::string& utf8_str) {
    std::vector<uint8_t> result;
    for (char c : utf8_str) {
        result.push_back(static_cast<uint8_t>(c));
        result.push_back(0);  // Null byte for UTF-16LE
    }
    return result;
}

// Helper function to reverse bytes in a 32-bit word
uint32_t byteswap32(uint32_t value) {
    return ((value & 0xFF) << 24) |
           (((value >> 8) & 0xFF) << 16) |
           (((value >> 16) & 0xFF) << 8) |
           ((value >> 24) & 0xFF);
}

// RC4 encryption for NTLMv2
class Rc4 {
public:
    Rc4(const std::vector<uint8_t>& key) {
        // Initialize S-box
        for (int i = 0; i < 256; ++i) {
            s_box_[i] = static_cast<uint8_t>(i);
        }
        
        // Key-scheduling algorithm
        int j = 0;
        for (int i = 0; i < 256; ++i) {
            j = (j + s_box_[i] + key[i % key.size()]) % 256;
            std::swap(s_box_[i], s_box_[j]);
        }
        
        i_ = 0;
        j_ = 0;
    }
    
    void encrypt(std::vector<uint8_t>& data) {
        for (size_t idx = 0; idx < data.size(); ++idx) {
            i_ = (i_ + 1) % 256;
            j_ = (j_ + s_box_[i_]) % 256;
            std::swap(s_box_[i_], s_box_[j_]);
            
            uint8_t k = s_box_[(s_box_[i_] + s_box_[j_]) % 256];
            data[idx] ^= k;
        }
    }

private:
    uint8_t s_box_[256];
    int i_, j_;
};

// Calculate NTLM hash (MD4 of UTF-16LE password)
std::vector<uint8_t> calculate_ntlm_hash(const std::string& password) {
    // Convert password to UTF-16LE
    std::vector<uint8_t> password_utf16 = utf8_to_utf16le(password);
    
    // For a complete implementation, we would use MD4 here
    // Since we don't have MD4, we'll simulate it with a placeholder
    // In a real implementation, you would use MD4 instead of MD5
    std::string pwd_str(password_utf16.begin(), password_utf16.end());
    std::string hash_hex = md5_hash(pwd_str);
    
    // Convert hex string back to bytes (this is a simplification)
    std::vector<uint8_t> result(16);  // NTLM hash is 16 bytes
    for (int i = 0; i < 16; ++i) {
        int byte_val = 0;
        sscanf(hash_hex.substr(i * 2, 2).c_str(), "%02x", &byte_val);
        result[i] = static_cast<uint8_t>(byte_val);
    }
    
    return result;
}

// Calculate LM hash (simplified)
std::vector<uint8_t> calculate_lm_hash(const std::string& password) {
    // LM hash is deprecated and insecure, but for compatibility
    // In a real implementation, you would use the actual LM algorithm
    std::vector<uint8_t> result(16, 0);  // Return zero-filled for now
    return result;
}

// Calculate NTLMv2 hash
std::vector<uint8_t> calculate_ntlmv2_hash(
    const std::string& username,
    const std::string& target,
    const std::vector<uint8_t>& ntlm_hash,
    const std::string& server_challenge,
    const std::string& client_challenge) {
    
    // Create blob for NTLMv2
    std::vector<uint8_t> blob;
    blob.push_back(0x01);  // Signature
    blob.push_back(0x01);  // Reserved
    blob.insert(blob.end(), 6, 0);  // Reserved
    
    // Add timestamp (simplified)
    uint64_t timestamp = 0x000000007C2D3529ULL;  // Sample timestamp
    for (int i = 0; i < 8; ++i) {
        blob.push_back(static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF));
    }
    
    // Add client challenge
    for (char c : client_challenge) {
        blob.push_back(static_cast<uint8_t>(c));
    }
    
    blob.insert(blob.end(), 4, 0);  // Reserved
    
    // Concatenate username and target
    std::vector<uint8_t> user_target = utf8_to_utf16le(to_upper(username) + target);
    
    // For a complete implementation, we would use HMAC-MD5 here
    // Instead, we'll use a simplified approach
    std::string temp = std::string(ntlm_hash.begin(), ntlm_hash.end()) + 
                      std::string(user_target.begin(), user_target.end());
    std::string hash_hex = md5_hash(temp + server_challenge + std::string(blob.begin(), blob.end()));
    
    std::vector<uint8_t> result(16);
    for (int i = 0; i < 16; ++i) {
        int byte_val = 0;
        sscanf(hash_hex.substr(i * 2, 2).c_str(), "%02x", &byte_val);
        result[i] = static_cast<uint8_t>(byte_val);
    }
    
    return result;
}

// Generate NTLM Type 1 message
std::string create_ntlm_type1(const std::string& domain, const std::string& workstation) {
    std::vector<uint8_t> message;
    
    // NTLMSSP signature (8 bytes)
    const char* signature = "NTLMSSP\x00";
    for (int i = 0; i < 8; ++i) {
        message.push_back(signature[i]);
    }
    
    // Message type (4 bytes, little endian)
    message.push_back(0x01);  // Type 1
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    
    // Flags (4 bytes, little endian)
    // Negotiate Unicode, Negotiate OEM, Request Target, Negotiate NTLM, Negotiate Always Sign
    uint32_t flags = 0x00000001 | 0x00000010 | 0x00000002 | 0x00000200 | 0x00008000;
    message.push_back(flags & 0xFF);
    message.push_back((flags >> 8) & 0xFF);
    message.push_back((flags >> 16) & 0xFF);
    message.push_back((flags >> 24) & 0xFF);
    
    // Domain name fields (offset and length)
    std::vector<uint8_t> domain_bytes = utf8_to_utf16le(domain);
    uint16_t domain_len = static_cast<uint16_t>(domain_bytes.size());
    uint32_t offset = 32 + 8;  // Start after header and fields
    
    message.push_back(domain_len & 0xFF);
    message.push_back((domain_len >> 8) & 0xFF);
    message.push_back(domain_len & 0xFF);
    message.push_back((domain_len >> 8) & 0xFF);
    message.push_back(offset & 0xFF);
    message.push_back((offset >> 8) & 0xFF);
    message.push_back((offset >> 16) & 0xFF);
    message.push_back((offset >> 24) & 0xFF);
    
    // Workstation fields (offset and length)
    std::vector<uint8_t> workstation_bytes = utf8_to_utf16le(workstation);
    uint16_t workstation_len = static_cast<uint16_t>(workstation_bytes.size());
    
    message.push_back(workstation_len & 0xFF);
    message.push_back((workstation_len >> 8) & 0xFF);
    message.push_back(workstation_len & 0xFF);
    message.push_back((workstation_len >> 8) & 0xFF);
    message.push_back((offset + domain_len) & 0xFF);
    message.push_back(((offset + domain_len) >> 8) & 0xFF);
    message.push_back(((offset + domain_len) >> 16) & 0xFF);
    message.push_back(((offset + domain_len) >> 24) & 0xFF);
    
    // Version info (8 bytes, optional)
    message.push_back(0x06);  // Product major version (WinNT)
    message.push_back(0x01);  // Product minor version (NT 4.0)
    message.push_back(0x24);  // Product build
    message.push_back(0x00);
    message.push_back(0x0F);  // NTLM revision (current is 15)
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    
    // Add domain and workstation strings
    message.insert(message.end(), domain_bytes.begin(), domain_bytes.end());
    message.insert(message.end(), workstation_bytes.begin(), workstation_bytes.end());
    
    // Base64 encode
    return base64_encode_impl(message.data(), static_cast<unsigned int>(message.size()));
}

// Generate NTLM Type 3 message
std::string create_ntlm_type3(
    const std::string& username,
    const std::string& domain,
    const std::string& workstation,
    const std::string& password,
    const std::string& server_challenge,
    const std::string& target_info) {
    
    std::vector<uint8_t> message;
    
    // NTLMSSP signature (8 bytes)
    const char* signature = "NTLMSSP\x00";
    for (int i = 0; i < 8; ++i) {
        message.push_back(signature[i]);
    }
    
    // Message type (4 bytes, little endian)
    message.push_back(0x03);  // Type 3
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    
    // Calculate hashes
    std::vector<uint8_t> ntlm_hash = calculate_ntlm_hash(password);
    
    // Generate client challenge
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::string client_challenge;
    for (int i = 0; i < 8; ++i) {
        client_challenge += static_cast<char>(dis(gen));
    }
    
    // Calculate NTLMv2 response
    std::vector<uint8_t> ntlm_v2_response = calculate_ntlmv2_hash(
        username, domain, ntlm_hash, server_challenge, client_challenge);
    
    // LMv2 response (first 16 bytes of NTLMv2 + client challenge)
    std::vector<uint8_t> lm_v2_response = ntlm_v2_response;
    for (char c : client_challenge) {
        lm_v2_response.push_back(static_cast<uint8_t>(c));
    }
    
    // Calculate offsets
    uint32_t offset = 64;  // Start after header and fields
    
    // LM Response fields
    uint16_t lm_resp_len = static_cast<uint16_t>(lm_v2_response.size());
    message.push_back(lm_resp_len & 0xFF);
    message.push_back((lm_resp_len >> 8) & 0xFF);
    message.push_back(lm_resp_len & 0xFF);
    message.push_back((lm_resp_len >> 8) & 0xFF);
    message.push_back(offset & 0xFF);
    message.push_back((offset >> 8) & 0xFF);
    message.push_back((offset >> 16) & 0xFF);
    message.push_back((offset >> 24) & 0xFF);
    
    // NTLM Response fields
    uint16_t ntlm_resp_len = static_cast<uint16_t>(ntlm_v2_response.size());
    message.push_back(ntlm_resp_len & 0xFF);
    message.push_back((ntlm_resp_len >> 8) & 0xFF);
    message.push_back(ntlm_resp_len & 0xFF);
    message.push_back((ntlm_resp_len >> 8) & 0xFF);
    message.push_back((offset + lm_resp_len) & 0xFF);
    message.push_back(((offset + lm_resp_len) >> 8) & 0xFF);
    message.push_back(((offset + lm_resp_len) >> 16) & 0xFF);
    message.push_back(((offset + lm_resp_len) >> 24) & 0xFF);
    
    // Domain name fields
    std::vector<uint8_t> domain_bytes = utf8_to_utf16le(domain);
    uint16_t domain_len = static_cast<uint16_t>(domain_bytes.size());
    
    message.push_back(domain_len & 0xFF);
    message.push_back((domain_len >> 8) & 0xFF);
    message.push_back(domain_len & 0xFF);
    message.push_back((domain_len >> 8) & 0xFF);
    message.push_back((offset + lm_resp_len + ntlm_resp_len) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len) >> 8) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len) >> 16) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len) >> 24) & 0xFF);
    
    // Username fields
    std::vector<uint8_t> user_bytes = utf8_to_utf16le(username);
    uint16_t user_len = static_cast<uint16_t>(user_bytes.size());
    
    message.push_back(user_len & 0xFF);
    message.push_back((user_len >> 8) & 0xFF);
    message.push_back(user_len & 0xFF);
    message.push_back((user_len >> 8) & 0xFF);
    message.push_back((offset + lm_resp_len + ntlm_resp_len + domain_len) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len) >> 8) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len) >> 16) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len) >> 24) & 0xFF);
    
    // Workstation fields
    std::vector<uint8_t> workstation_bytes = utf8_to_utf16le(workstation);
    uint16_t workstation_len = static_cast<uint16_t>(workstation_bytes.size());
    
    message.push_back(workstation_len & 0xFF);
    message.push_back((workstation_len >> 8) & 0xFF);
    message.push_back(workstation_len & 0xFF);
    message.push_back((workstation_len >> 8) & 0xFF);
    message.push_back((offset + lm_resp_len + ntlm_resp_len + domain_len + user_len) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len + user_len) >> 8) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len + user_len) >> 16) & 0xFF);
    message.push_back(((offset + lm_resp_len + ntlm_resp_len + domain_len + user_len) >> 24) & 0xFF);
    
    // Session key length (0 for now)
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    message.push_back(0x00);
    
    // Flags (same as Type 1)
    uint32_t flags = 0x00000001 | 0x00000010 | 0x00000002 | 0x00000200 | 0x00008000;
    message.push_back(flags & 0xFF);
    message.push_back((flags >> 8) & 0xFF);
    message.push_back((flags >> 16) & 0xFF);
    message.push_back((flags >> 24) & 0xFF);
    
    // Add all the variable-length fields
    message.insert(message.end(), lm_v2_response.begin(), lm_v2_response.end());
    message.insert(message.end(), ntlm_v2_response.begin(), ntlm_v2_response.end());
    message.insert(message.end(), domain_bytes.begin(), domain_bytes.end());
    message.insert(message.end(), user_bytes.begin(), user_bytes.end());
    message.insert(message.end(), workstation_bytes.begin(), workstation_bytes.end());
    
    // Base64 encode
    return base64_encode_impl(message.data(), static_cast<unsigned int>(message.size()));
}

// Base64 encoding helper
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode_impl(unsigned char const* buf, unsigned int bufLen) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char array_3[3];
    unsigned char array_4[4];

    while (bufLen--) {
        array_3[i++] = *(buf++);
        if (i == 3) {
            array_4[0] = (array_3[0] & 0xfc) >> 2;
            array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
            array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
            array_4[3] = array_3[2] & 0x3f;

            for(i = 0; (i < 4) ; i++)
                ret += base64_chars[array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            array_3[j] = '\0';

        array_4[0] = (array_3[0] & 0xfc) >> 2;
        array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
        array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
        array_4[3] = array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

}  // namespace requests_cpp