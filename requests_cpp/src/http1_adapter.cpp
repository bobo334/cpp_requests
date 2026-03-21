#include "requests_cpp/adapter.hpp"
#include "requests_cpp/exceptions.hpp"
#include "requests_cpp/http1.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/tls.hpp"
#include "requests_cpp/url.hpp"

#include "connection_pool.hpp"
#include "tls_connection_pool.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <sstream>
#include <string>

namespace requests_cpp {

namespace {
bool has_complete_response(const std::string& raw, const std::string& method, bool eof) {
    auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }

    std::string head = raw.substr(0, header_end);
    Headers headers;
    std::istringstream iss(head);
    std::string line;
    std::getline(iss, line);  // status line

    int status_code = 0;
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    if (!line.empty()) {
        std::istringstream status_stream(line);
        std::string http_version;
        status_stream >> http_version >> status_code;
    }

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
        if (!value.empty() && value.front() == ' ') {
            value.erase(0, 1);
        }
        headers[key] = value;
    }

    std::size_t body_end = requests_cpp::http1::find_complete_body_end(raw, header_end, headers, status_code, method);
    if (body_end != std::string::npos) {
        return true;
    }

    if (!eof) {
        return false;
    }

    auto te_it = headers.find("Transfer-Encoding");
    if (te_it != headers.end()) {
        std::string enc = te_it->second;
        std::transform(enc.begin(), enc.end(), enc.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (enc.find("chunked") != std::string::npos) {
            return false;
        }
    }

    auto length_it = headers.find("Content-Length");
    if (length_it != headers.end()) {
        return false;
    }

    return header_end != std::string::npos;
}

bool should_keep_alive(const Request& request, const Response& response) {
    auto conn_req = request.headers().find("Connection");
    if (conn_req != request.headers().end()) {
        std::string value = conn_req->second;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (value.find("close") != std::string::npos) {
            return false;
        }
    }

    auto conn_resp = response.headers().find("Connection");
    if (conn_resp != response.headers().end()) {
        std::string value = conn_resp->second;
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        if (value.find("close") != std::string::npos) {
            return false;
        }
    }

    return true;
}

class Http1Adapter : public Adapter {
public:
    Http1Adapter() = default;
    explicit Http1Adapter(http1::ConnectionPoolConfig config) : pool_(std::move(config)) {}

    Response send(const Request& request) override {
        Url url = Url::parse(request.url());
        auto raw_request = http1::serialize_request(request, url);

        ErrorContext context{};
        context.request_url = request.url();
        context.method = request.method();

        if (url.scheme == "https") {
            tls::TlsConfig config{};
            config.read_timeout_ms = request.timeout().read_ms;
            config.write_timeout_ms = request.timeout().read_ms;
            config.connect_timeout_ms = request.timeout().connect_ms;

            std::string error;
            tls::TlsClient tls_client = tls_pool_.acquire(url, config, error);
            if (!tls_client.is_open()) {
                context.tls_info = error;
                throw SSLError(ErrorCode::TLS, error, context);
            }

            if (tls_client.send(raw_request) == 0) {
                tls_pool_.release(url, std::move(tls_client), false);
                throw IOError(ErrorCode::IO, "TLS send failed", context);
            }

            std::string raw_response;
            bool eof = false;
            while (true) {
                auto chunk = tls_client.recv_some();
                if (chunk.empty()) {
                    eof = true;
                    break;
                }
                raw_response += chunk;
                if (has_complete_response(raw_response, request.method(), false)) {
                    break;
                }
            }

            Response response = http1::parse_response(raw_response);
            bool response_complete = has_complete_response(raw_response, request.method(), eof);
            bool keep_alive = response_complete && !eof && response.status_code() != 0 && should_keep_alive(request, response);
            tls_pool_.release(url, std::move(tls_client), keep_alive);
            return response;
        }

        http1::Connection connection = pool_.acquire(url, request.timeout());

        if (!connection.is_open()) {
            throw ConnectionError(ErrorCode::Network, "connection failed", context);
        }

        std::size_t sent = connection.send_all(raw_request);
        if (sent != raw_request.size()) {
            connection.close();
            connection = pool_.acquire(url, request.timeout());
            if (!connection.is_open()) {
                throw ConnectionError(ErrorCode::Network, "connection failed", context);
            }
            sent = connection.send_all(raw_request);
            if (sent != raw_request.size()) {
                connection.close();
                throw IOError(ErrorCode::IO, "send failed", context);
            }
        }

        std::string raw_response;
        bool eof = false;
        while (true) {
            auto chunk = connection.recv_some(request.timeout());
            if (chunk.empty()) {
                eof = true;
                break;
            }
            raw_response += chunk;
            if (has_complete_response(raw_response, request.method(), false)) {
                break;
            }
        }

        Response response = http1::parse_response(raw_response);
        bool response_complete = has_complete_response(raw_response, request.method(), eof);
        bool keep_alive = response_complete && !eof && response.status_code() != 0 && should_keep_alive(request, response);
        pool_.release(std::move(connection), keep_alive);
        return response;
    }

private:
    http1::ConnectionPool pool_{};
    tls::TlsConnectionPool tls_pool_{};
};
}  // namespace

std::shared_ptr<Adapter> create_http1_adapter() {
    return std::make_shared<Http1Adapter>();
}

std::shared_ptr<Adapter> create_http1_adapter(http1::ConnectionPoolConfig config) {
    return std::make_shared<Http1Adapter>(std::move(config));
}

}  // namespace requests_cpp
