#include "requests_cpp/http1.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {
std::vector<unsigned char> decode_base64(const std::string& input) {
    static const int kInvalid = -1;
    static int table[256] = {0};
    static bool initialized = false;
    if (!initialized) {
        std::fill(std::begin(table), std::end(table), kInvalid);
        for (int i = 'A'; i <= 'Z'; ++i) table[i] = i - 'A';
        for (int i = 'a'; i <= 'z'; ++i) table[i] = 26 + i - 'a';
        for (int i = '0'; i <= '9'; ++i) table[i] = 52 + i - '0';
        table[static_cast<unsigned char>('+')] = 62;
        table[static_cast<unsigned char>('/')] = 63;
        initialized = true;
    }

    std::vector<unsigned char> output;
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (c == '=') {
            break;
        }
        int d = table[c];
        if (d == kInvalid) {
            continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}
}  // namespace

int main() {
    const std::string payload_base64 = "eJzj5m+Q7+Zg6E3cmsn0/7TH2ZMnwzXO65/yZGRoFfTi5WZgYGAGAOgLC1g=";
    const auto payload_bytes = decode_base64(payload_base64);
    std::string payload(reinterpret_cast<const char*>(payload_bytes.data()), payload_bytes.size());

    const auto response = requests_cpp::http1::parse_response(
        "HTTP/1.1 200 OK\r\n"
        "Content-Encoding: deflate, br, gzip\r\n"
        "Content-Length: " + std::to_string(payload.size()) + "\r\n"
        "\r\n" + payload);

    std::cout << response.text() << "\n";
    return 0;
}
