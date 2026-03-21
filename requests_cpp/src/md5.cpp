#include "requests_cpp/md5.hpp"
#include <cstring>
#include <algorithm>

namespace requests_cpp {

// MD5 implementation constants
static const uint32_t kMd5Constants[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint32_t kMd5Shifts[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

// Helper functions
inline uint32_t left_rotate(uint32_t value, uint32_t shift) {
    return (value << shift) | (value >> (32 - shift));
}

inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) | (~x & z);
}

inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) {
    return (x & z) | (y & ~z);
}

inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) {
    return x ^ y ^ z;
}

inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) {
    return y ^ (x | ~z);
}

inline uint32_t apply_round(uint32_t func_result, uint32_t state, uint32_t input_word, uint32_t constant, uint32_t shift) {
    uint32_t result = state + left_rotate(func_result + state + input_word + constant, shift);
    return result;
}

// MD5 implementation
std::string md5_hash(const std::string& input) {
    // Convert input string to bytes
    std::vector<uint8_t> data(input.begin(), input.end());
    
    // Append padding bits
    size_t original_length = data.size();
    size_t original_bit_length = original_length * 8;
    
    // Append '1' bit (represented as 0x80 byte)
    data.push_back(0x80);
    
    // Append zeros until length ≡ 448 (mod 512)
    while ((data.size() * 8) % 512 != 448) {
        data.push_back(0x00);
    }
    
    // Append original length as 64-bit little-endian integer
    for (int i = 0; i < 8; ++i) {
        data.push_back(static_cast<uint8_t>((original_bit_length >> (i * 8)) & 0xFF));
    }
    
    // Initialize MD5 state
    uint32_t a0 = 0x67452301;  // A
    uint32_t b0 = 0xefcdab89;  // B
    uint32_t c0 = 0x98badcfe;  // C
    uint32_t d0 = 0x10325476;  // D
    
    // Process each 512-bit chunk
    for (size_t i = 0; i < data.size(); i += 64) {  // 64 bytes = 512 bits
        // Copy chunk into array of 32-bit words
        uint32_t chunk[16];
        for (int j = 0; j < 16; ++j) {
            chunk[j] = (static_cast<uint32_t>(data[i + j * 4])) |
                      (static_cast<uint32_t>(data[i + j * 4 + 1]) << 8) |
                      (static_cast<uint32_t>(data[i + j * 4 + 2]) << 16) |
                      (static_cast<uint32_t>(data[i + j * 4 + 3]) << 24);
        }
        
        // Save current state
        uint32_t aa = a0;
        uint32_t bb = b0;
        uint32_t cc = c0;
        uint32_t dd = d0;
        
        // Main loop
        for (int i_round = 0; i_round < 64; ++i_round) {
            uint32_t f, g;
            
            if (i_round < 16) {
                f = F(bb, cc, dd);
                g = i_round;
            } else if (i_round < 32) {
                f = G(bb, cc, dd);
                g = (5 * i_round + 1) % 16;
            } else if (i_round < 48) {
                f = H(bb, cc, dd);
                g = (3 * i_round + 5) % 16;
            } else {
                f = I(bb, cc, dd);
                g = (7 * i_round) % 16;
            }
            
            f = f + aa + kMd5Constants[i_round] + chunk[g];
            aa = dd;
            dd = cc;
            cc = bb;
            bb = bb + left_rotate(f, kMd5Shifts[i_round]);
        }
        
        // Add this chunk's hash to result so far
        a0 += aa;
        b0 += bb;
        c0 += cc;
        d0 += dd;
    }
    
    // Produce final hash value as little-endian bytes
    char hash[16];
    for (int i = 0; i < 4; ++i) {
        uint32_t val = (i == 0) ? a0 : (i == 1) ? b0 : (i == 2) ? c0 : d0;
        hash[i * 4] = val & 0xFF;
        hash[i * 4 + 1] = (val >> 8) & 0xFF;
        hash[i * 4 + 2] = (val >> 16) & 0xFF;
        hash[i * 4 + 3] = (val >> 24) & 0xFF;
    }
    
    // Convert to hex string
    char hex_digits[] = "0123456789abcdef";
    std::string result;
    for (int i = 0; i < 16; ++i) {
        result += hex_digits[(hash[i] >> 4) & 0xF];
        result += hex_digits[hash[i] & 0xF];
    }
    
    return result;
}

// Utility function to convert string to uppercase
std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Utility function to trim whitespace
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

}  // namespace requests_cpp