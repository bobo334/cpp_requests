#include "requests_cpp/session.hpp"
#include "requests_cpp/http2_smoke.hpp"

#include <iostream>

int main() {
    requests_cpp::Session session;
    session.enable_http2(true);

    auto response = session.get("https://nghttp2.org/httpbin/get");
    std::cout << "status=" << response.status_code() << "\n";
    std::cout << "body=" << response.text() << "\n";

    requests_cpp::Request request("https://nghttp2.org/httpbin/get");
    auto response2 = requests_cpp::http2::http2_smoke_request(request);
    std::cout << "h2_status=" << response2.status_code() << "\n";
    std::cout << "h2_body=" << response2.text() << "\n";
    return 0;
}
