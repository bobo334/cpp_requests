#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace requests_cpp {

struct CaseInsensitiveLess {
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return std::lexicographical_compare(
            lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](unsigned char a, unsigned char b) {
                return std::tolower(a) < std::tolower(b);
            });
    }
};

using Headers = std::map<std::string, std::string, CaseInsensitiveLess>;
using Params = std::vector<std::pair<std::string, std::string>>;

struct Timeout {
    long connect_ms{0};
    long read_ms{0};
};

}  // namespace requests_cpp
