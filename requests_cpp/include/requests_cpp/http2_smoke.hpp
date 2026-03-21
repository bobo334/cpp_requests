#pragma once

#include "requests_cpp/http2.hpp"
#include "requests_cpp/response.hpp"
#include "requests_cpp/adapter.hpp"

namespace requests_cpp::http2 {

// Minimal HTTP/2 validation helper: attempt a single request and return response.
// Uses the Http2Adapter and assumes HTTPS target. This is intended for smoke tests.
Response http2_smoke_request(const Request& request);

}
