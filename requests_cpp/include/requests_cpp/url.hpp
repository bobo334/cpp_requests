#pragma once

#include <cstdint>
#include <string>

namespace requests_cpp {

struct Url {
    std::string scheme;
    std::string host;
    std::string path;
    std::string query;
    std::uint16_t port{0};
    bool has_port{false};

    std::string authority() const;
    std::string target() const;

    static Url parse(const std::string& url);
};

}  // namespace requests_cpp
