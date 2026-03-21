#include "requests_cpp/http1.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "miniz.h"
#include <brotli/decode.h>

namespace requests_cpp::http1 {

namespace {
bool header_exists(const Headers& headers, const std::string& key) {
    return headers.find(key) != headers.end();
}
}  // namespace

std::string serialize_request(const Request& request, const Url& url) {
    std::ostringstream oss;
    oss << request.method() << " " << url.target() << " HTTP/1.1\r\n";

    Headers headers = request.headers();
    if (!header_exists(headers, "Host")) {
        headers["Host"] = url.authority();
    }

    if (!header_exists(headers, "Connection")) {
        headers["Connection"] = "keep-alive";
    }

    if (!request.body().empty() && !header_exists(headers, "Content-Length")) {
        headers["Content-Length"] = std::to_string(request.body().size());
    }

    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n";
    oss << request.body();
    return oss.str();
}

namespace {
std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string trim(std::string value) {
    auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    auto begin = std::find_if_not(value.begin(), value.end(), is_space);
    auto end = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::size_t parse_content_length(const Headers& headers) {
    auto it = headers.find("Content-Length");
    if (it == headers.end()) {
        return 0;
    }
    try {
        return static_cast<std::size_t>(std::stoull(it->second));
    } catch (...) {
        return 0;
    }
}

struct ChunkDecodeResult {
    std::string decoded;
    Headers trailers;
};

bool parse_chunk_size(const std::string& line, std::size_t& size) {
    auto semi = line.find(';');
    std::string value = semi == std::string::npos ? line : line.substr(0, semi);
    value = trim(value);
    if (value.empty()) {
        return false;
    }
    try {
        size = static_cast<std::size_t>(std::stoul(value, nullptr, 16));
        return true;
    } catch (...) {
        return false;
    }
}

ChunkDecodeResult decode_chunked_with_trailers(const std::string& body) {
    ChunkDecodeResult result;
    std::size_t pos = 0;
    while (pos < body.size()) {
        auto line_end = body.find("\r\n", pos);
        if (line_end == std::string::npos) {
            break;
        }
        std::string size_str = body.substr(pos, line_end - pos);
        std::size_t chunk_size = 0;
        if (!parse_chunk_size(size_str, chunk_size)) {
            break;
        }
        pos = line_end + 2;
        if (chunk_size == 0) {
            break;
        }
        if (pos + chunk_size > body.size()) {
            break;
        }
        result.decoded.append(body, pos, chunk_size);
        pos += chunk_size;
        if (body.compare(pos, 2, "\r\n") != 0) {
            break;
        }
        pos += 2;
    }

    if (pos < body.size()) {
        if (body.compare(pos, 2, "\r\n") == 0) {
            pos += 2;
        }
    }

    while (pos < body.size()) {
        auto line_end = body.find("\r\n", pos);
        if (line_end == std::string::npos) {
            break;
        }
        if (line_end == pos) {
            pos += 2;
            break;
        }
        std::string line = body.substr(pos, line_end - pos);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            result.trailers[key] = trim(value);
        }
        pos = line_end + 2;
    }

    return result;
}

std::string decode_chunked(const std::string& body) {
    return decode_chunked_with_trailers(body).decoded;
}

std::string inflate_zlib(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    mz_stream stream{};
    stream.next_in = reinterpret_cast<const unsigned char*>(input.data());
    stream.avail_in = static_cast<unsigned int>(input.size());

    int status = mz_inflateInit(&stream);
    if (status != MZ_OK) {
        return {};
    }

    std::string output;
    std::vector<unsigned char> buffer(16 * 1024);
    do {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<unsigned int>(buffer.size());
        status = mz_inflate(&stream, MZ_NO_FLUSH);
        if (status != MZ_OK && status != MZ_STREAM_END) {
            mz_inflateEnd(&stream);
            return {};
        }
        std::size_t produced = buffer.size() - stream.avail_out;
        output.append(reinterpret_cast<const char*>(buffer.data()), produced);
    } while (status != MZ_STREAM_END);

    mz_inflateEnd(&stream);
    return output;
}

std::string inflate_gzip(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    mz_stream stream{};
    stream.next_in = reinterpret_cast<const unsigned char*>(input.data());
    stream.avail_in = static_cast<unsigned int>(input.size());

    int status = mz_inflateInit2(&stream, 16 + MZ_DEFAULT_WINDOW_BITS);
    if (status != MZ_OK) {
        return {};
    }

    std::string output;
    std::vector<unsigned char> buffer(16 * 1024);
    do {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<unsigned int>(buffer.size());
        status = mz_inflate(&stream, MZ_NO_FLUSH);
        if (status != MZ_OK && status != MZ_STREAM_END) {
            mz_inflateEnd(&stream);
            return {};
        }
        std::size_t produced = buffer.size() - stream.avail_out;
        output.append(reinterpret_cast<const char*>(buffer.data()), produced);
    } while (status != MZ_STREAM_END);

    mz_inflateEnd(&stream);
    return output;
}

std::string inflate_brotli(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    BrotliDecoderState* state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!state) {
        return {};
    }

    const uint8_t* next_in = reinterpret_cast<const uint8_t*>(input.data());
    size_t avail_in = input.size();
    std::string output;
    std::vector<uint8_t> buffer(16 * 1024);

    while (true) {
        uint8_t* next_out = buffer.data();
        size_t avail_out = buffer.size();
        BrotliDecoderResult result = BrotliDecoderDecompressStream(
            state, &avail_in, &next_in, &avail_out, &next_out, nullptr);

        std::size_t produced = buffer.size() - avail_out;
        if (produced > 0) {
            output.append(reinterpret_cast<const char*>(buffer.data()), produced);
        }

        if (result == BROTLI_DECODER_RESULT_SUCCESS) {
            break;
        }
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            continue;
        }
        if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
            if (avail_in == 0) {
                BrotliDecoderDestroyInstance(state);
                return {};
            }
            continue;
        }
        BrotliDecoderDestroyInstance(state);
        return {};
    }

    BrotliDecoderDestroyInstance(state);
    return output;
}

std::vector<std::string> split_content_encodings(const std::string& value) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start < value.size()) {
        auto comma = value.find(',', start);
        std::string token = comma == std::string::npos
            ? value.substr(start)
            : value.substr(start, comma - start);
        token = trim(token);
        if (!token.empty()) {
            result.push_back(to_lower(token));
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return result;
}

bool is_connection_close(const Headers& headers) {
    auto it = headers.find("Connection");
    if (it == headers.end()) {
        return false;
    }
    std::string value = to_lower(it->second);
    return value.find("close") != std::string::npos;
}

std::size_t find_complete_body_end_impl(const std::string& raw,
                                       std::size_t header_end,
                                       const Headers& headers,
                                       int status_code,
                                       const std::string& method) {
    if (header_end == std::string::npos) {
        return std::string::npos;
    }
    std::size_t body_start = header_end + 4;

    std::string method_lower = to_lower(method);
    if (method_lower == "head" || (status_code >= 100 && status_code < 200) || status_code == 204 || status_code == 304) {
        return body_start;
    }

    auto te_it = headers.find("Transfer-Encoding");
    if (te_it != headers.end()) {
        std::string enc = to_lower(te_it->second);
        if (enc.find("chunked") != std::string::npos) {
            auto terminator = raw.find("\r\n0\r\n\r\n", body_start);
            if (terminator == std::string::npos) {
                return std::string::npos;
            }
            return terminator + 5;
        }
    }

    auto length = parse_content_length(headers);
    if (length > 0) {
        if (raw.size() >= body_start + length) {
            return body_start + length;
        }
        return std::string::npos;
    }

    if (is_connection_close(headers)) {
        return raw.size();
    }

    return std::string::npos;
}
}  // namespace

Response parse_response(const std::string& raw) {
    Response response;
    response.set_raw(raw);

    auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        response.set_text(raw);
        return response;
    }

    std::string head = raw.substr(0, header_end);
    std::string body = raw.substr(header_end + 4);

    std::istringstream iss(head);
    std::string status_line;
    if (!std::getline(iss, status_line)) {
        response.set_text(body);
        return response;
    }

    if (!status_line.empty() && status_line.back() == '\r') {
        status_line.pop_back();
    }

    std::istringstream status_stream(status_line);
    std::string http_version;
    int status_code = 0;
    status_stream >> http_version >> status_code;
    response.set_status_code(status_code);

    Headers headers;
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        headers[key] = trim(value);
    }

    response.set_headers(headers);

    std::size_t body_end = find_complete_body_end_impl(raw, header_end, headers, response.status_code(), "");
    if (body_end == std::string::npos) {
        response.set_text(body);
        return response;
    }

    std::string body_view = raw.substr(header_end + 4, body_end - (header_end + 4));
    bool chunked_decoded = false;

    auto transfer_encoding_it = headers.find("Transfer-Encoding");
    if (transfer_encoding_it != headers.end()) {
        auto encoding = to_lower(transfer_encoding_it->second);
        if (encoding.find("chunked") != std::string::npos) {
            auto decoded = decode_chunked_with_trailers(body_view);
            body_view = decoded.decoded;
            chunked_decoded = true;
            if (!decoded.trailers.empty()) {
                Headers merged = headers;
                merged.insert(decoded.trailers.begin(), decoded.trailers.end());
                headers = std::move(merged);
                response.set_headers(headers);
            }
        }
    }

    auto encoding_it = headers.find("Content-Encoding");
    if (encoding_it != headers.end()) {
        auto encodings = split_content_encodings(encoding_it->second);
        if (!encodings.empty()) {
            std::string decoded = body_view;
            bool ok = true;
            for (auto it = encodings.rbegin(); it != encodings.rend(); ++it) {
                const auto& encoding = *it;
                if (encoding == "identity") {
                    continue;
                }
                std::string next;
                if (encoding == "gzip") {
                    next = inflate_gzip(decoded);
                } else if (encoding == "deflate") {
                    next = inflate_zlib(decoded);
                } else if (encoding == "br") {
                    next = inflate_brotli(decoded);
                } else {
                    ok = false;
                    break;
                }
                if (next.empty()) {
                    ok = false;
                    break;
                }
                decoded = std::move(next);
            }
            if (ok) {
                response.set_text(decoded);
                return response;
            }
        }
        response.set_text(body_view);
        return response;
    }

    if (!chunked_decoded) {
        auto length = parse_content_length(headers);
        if (length > 0 && body_view.size() >= length) {
            response.set_text(body_view.substr(0, length));
            return response;
        }
    }

    response.set_text(body_view);
    return response;
}

std::size_t find_complete_body_end(const std::string& raw, std::size_t header_end, const Headers& headers) {
    return find_complete_body_end_impl(raw, header_end, headers, 0, "");
}

std::size_t find_complete_body_end(const std::string& raw,
                                   std::size_t header_end,
                                   const Headers& headers,
                                   int status_code,
                                   const std::string& method) {
    return find_complete_body_end_impl(raw, header_end, headers, status_code, method);
}

}  // namespace requests_cpp::http1
