#include "requests_cpp/api.hpp"

namespace requests_cpp {

Response request(const std::string& method, const std::string& url) {
    Session session;
    return session.request(method, url);
}

Response request(const std::string& method, const std::string& url, const std::string& body) {
    Session session;
    return session.request(method, url, body);
}

Response request(const std::string& method, const std::string& url, const std::string& body, const Timeout& timeout,
                 int retries) {
    Session session;
    return session.request(method, url, body, timeout, retries);
}

Response get(const std::string& url) {
    return request("GET", url);
}

Response post(const std::string& url) {
    return request("POST", url);
}

Response post(const std::string& url, const std::string& body) {
    return request("POST", url, body);
}

}  // namespace requests_cpp
