#include "requests_cpp/url.hpp"

#include "requests_cpp/exceptions.hpp"

#include <cctype>
#include <exception>

namespace requests_cpp {

namespace {
std::string to_lower(std::string value) {
    for (auto& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

void split_host_port(const std::string& input, std::string& host, std::uint16_t& port, bool& has_port) {
    has_port = false;
    host.clear();
    port = 0;

    if (input.empty()) {
        return;
    }

    if (input.front() == '[') {
        auto end = input.find(']');
        if (end == std::string::npos) {
            host = input;
            return;
        }
        host = input.substr(1, end - 1);
        if (end + 1 < input.size() && input[end + 1] == ':') {
            has_port = true;
            port = static_cast<std::uint16_t>(std::stoi(input.substr(end + 2)));
        }
        return;
    }

    auto colon = input.find(':');
    if (colon == std::string::npos) {
        host = input;
        return;
    }

    host = input.substr(0, colon);
    has_port = true;
    port = static_cast<std::uint16_t>(std::stoi(input.substr(colon + 1)));
}
}  // namespace

std::string Url::authority() const {
    if (!has_port) {
        return host;
    }
    return host + ":" + std::to_string(port);
}

std::string Url::target() const {
    std::string target = path.empty() ? "/" : path;
    if (!query.empty()) {
        target += "?" + query;
    }
    return target;
}

Url Url::parse(const std::string& url) {
    try {
        Url result;
        auto scheme_pos = url.find("://");
        if (scheme_pos == std::string::npos) {
            throw InvalidSchemaError(ErrorCode::Protocol, "missing URL scheme");
        }

        result.scheme = to_lower(url.substr(0, scheme_pos));
        if (result.scheme != "http" && result.scheme != "https") {
            ErrorContext context{};
            context.request_url = url;
            throw InvalidSchemaError(ErrorCode::Protocol, "unsupported URL scheme", context);
        }

        auto rest = url.substr(scheme_pos + 3);

        auto path_pos = rest.find('/');
        auto query_pos = rest.find('?');
        auto authority_end = std::min(path_pos == std::string::npos ? rest.size() : path_pos,
                                      query_pos == std::string::npos ? rest.size() : query_pos);
        auto authority = rest.substr(0, authority_end);

        split_host_port(authority, result.host, result.port, result.has_port);
        if (result.host.empty()) {
            ErrorContext context{};
            context.request_url = url;
            throw InvalidURLError(ErrorCode::Protocol, "missing URL host", context);
        }

        if (path_pos != std::string::npos) {
            auto path_end = query_pos == std::string::npos ? rest.size() : query_pos;
            result.path = rest.substr(path_pos, path_end - path_pos);
        } else {
            result.path = "/";
        }

        if (query_pos != std::string::npos) {
            result.query = rest.substr(query_pos + 1);
        }

        return result;
    } catch (const RequestException&) {
        throw;
    } catch (const std::exception& ex) {
        ErrorContext context{};
        context.request_url = url;
        throw InvalidURLError(ErrorCode::Protocol, ex.what(), context);
    }
}

}  // namespace requests_cpp
