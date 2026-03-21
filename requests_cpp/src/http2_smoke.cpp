#include "requests_cpp/http2_smoke.hpp"

namespace requests_cpp::http2 {

Response http2_smoke_request(const Request& request) {
    auto result = create_http2_adapter();
    if (!result.adapter || result.fallback_to_http1) {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::HTTP2);
        response.set_text(result.error.empty() ? "HTTP/2 adapter unavailable" : result.error);
        response.set_error_url(request.url());
        return response;
    }
    return result.adapter->send(request);
}

}  // namespace requests_cpp::http2
