#include "requests_cpp/http1.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
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

std::string to_hex_size(std::size_t value) {
    std::ostringstream oss;
    oss << std::hex << value;
    return oss.str();
}
}  // namespace

int main() {
    const std::string payload_base64 = "eJzj5m+Q7+Zg6E3cmsn0/7TH2ZMnwzXO65/yZGRoFfTi5WZgYGAGAOgLC1g=";
    const auto payload_bytes = decode_base64(payload_base64);
    const std::string payload(reinterpret_cast<const char*>(payload_bytes.data()), payload_bytes.size());

    std::string chunked_body;
    const std::string chunk_size = to_hex_size(payload.size());
    chunked_body.append(chunk_size).append("\r\n");
    chunked_body.append(payload).append("\r\n");
    chunked_body.append("0\r\n");
    chunked_body.append("X-Trailer: ok\r\n\r\n");

    const std::string raw_response =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Encoding: deflate, br, gzip\r\n"
        "\r\n" + chunked_body;

    const auto response = requests_cpp::http1::parse_response(raw_response);
    std::cout << response.text() << "\n";
    auto it = response.headers().find("X-Trailer");
    if (it != response.headers().end()) {
        std::cout << it->first << ":" << it->second << "\n";
    }

    return 0;
}
