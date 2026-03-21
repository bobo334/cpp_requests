#include "requests_cpp/http2.hpp"

#if defined(_WIN32)
#include <BaseTsd.h>
#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#define _SSIZE_T_DEFINED
#endif
#endif

#include <nghttp2/nghttp2.h>

#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include "requests_cpp/exceptions.hpp"
#include "requests_cpp/tls.hpp"

#include "tls_connection_pool.hpp"

namespace requests_cpp::http2 {

struct Http2Adapter::Impl {
    Http2Config config{};
    tls::TlsConnectionPool tls_pool{};
};

namespace {
struct StreamResponse {
    int status_code{0};
    Headers headers{};
    std::string body{};
    ErrorCode error_code{ErrorCode::Unknown};
    std::string error_message{};
};

struct BodyPayload {
    std::string data;
    std::size_t offset{0};
};

struct SessionContext {
    nghttp2_session* session{nullptr};
    tls::TlsClient tls_client{};
    Url url{};
    std::string send_buffer{};
    std::map<int32_t, StreamResponse> responses{};
    std::mutex mutex{};
};

int on_frame_recv_callback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data) {
    (void)session;
    auto* ctx = static_cast<SessionContext*>(user_data);
    if (!ctx) {
        return 0;
    }
    if (frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
        ctx->responses[frame->hd.stream_id];
    }
    return 0;
}

int on_header_callback(nghttp2_session* session,
                       const nghttp2_frame* frame,
                       const uint8_t* name,
                       size_t namelen,
                       const uint8_t* value,
                       size_t valuelen,
                       uint8_t flags,
                       void* user_data) {
    (void)session;
    (void)flags;
    auto* ctx = static_cast<SessionContext*>(user_data);
    if (!ctx) {
        return 0;
    }
    if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_RESPONSE) {
        return 0;
    }
    std::string key(reinterpret_cast<const char*>(name), namelen);
    std::string val(reinterpret_cast<const char*>(value), valuelen);
    auto& resp = ctx->responses[frame->hd.stream_id];
    if (key == ":status") {
        resp.status_code = std::atoi(val.c_str());
    } else {
        resp.headers[key] = val;
    }
    return 0;
}

int on_data_chunk_recv_callback(nghttp2_session* session,
                                uint8_t flags,
                                int32_t stream_id,
                                const uint8_t* data,
                                size_t len,
                                void* user_data) {
    (void)session;
    (void)flags;
    auto* ctx = static_cast<SessionContext*>(user_data);
    if (!ctx) {
        return 0;
    }
    auto& resp = ctx->responses[stream_id];
    resp.body.append(reinterpret_cast<const char*>(data), len);
    return 0;
}

int on_stream_close_callback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data) {
    (void)session;
    auto* ctx = static_cast<SessionContext*>(user_data);
    if (!ctx) {
        return 0;
    }
    auto it = ctx->responses.find(stream_id);
    if (it != ctx->responses.end()) {
        if (error_code != NGHTTP2_NO_ERROR && it->second.error_code == ErrorCode::Unknown) {
            it->second.error_code = ErrorCode::HTTP2;
            it->second.error_message = nghttp2_http2_strerror(error_code);
        }
        auto* payload = static_cast<void*>(nghttp2_session_get_stream_user_data(session, stream_id));
        if (payload) {
            delete static_cast<BodyPayload*>(payload);
            nghttp2_session_set_stream_user_data(session, stream_id, nullptr);
        }
    }
    return 0;
}

ssize_t send_callback(nghttp2_session* session, const uint8_t* data, size_t length, int flags, void* user_data) {
    (void)session;
    (void)flags;
    auto* ctx = static_cast<SessionContext*>(user_data);
    if (!ctx) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    ctx->send_buffer.append(reinterpret_cast<const char*>(data), length);
    return static_cast<ssize_t>(length);
}

bool flush_send_buffer(SessionContext& ctx) {
    if (ctx.send_buffer.empty()) {
        return true;
    }
    std::size_t sent = ctx.tls_client.send(ctx.send_buffer);
    ctx.send_buffer.clear();
    return sent > 0;
}

bool recv_into_session(SessionContext& ctx) {
    auto chunk = ctx.tls_client.recv_some();
    if (chunk.empty()) {
        return false;
    }
    nghttp2_ssize rv = nghttp2_session_mem_recv(ctx.session,
                                                reinterpret_cast<const uint8_t*>(chunk.data()),
                                                chunk.size());
    return rv >= 0;
}

std::string build_authority(const Url& url) {
    if (url.has_port) {
        return url.host + ":" + std::to_string(url.port);
    }
    return url.host;
}

std::vector<nghttp2_nv> build_headers(const Request& request, const Url& url) {
    std::vector<nghttp2_nv> headers;
    auto add = [&](const std::string& name, const std::string& value) {
        nghttp2_nv nv{};
        nv.name = reinterpret_cast<uint8_t*>(const_cast<char*>(name.data()));
        nv.value = reinterpret_cast<uint8_t*>(const_cast<char*>(value.data()));
        nv.namelen = name.size();
        nv.valuelen = value.size();
        nv.flags = NGHTTP2_NV_FLAG_NONE;
        headers.push_back(nv);
    };

    std::string path = url.path.empty() ? "/" : url.path;
    if (!url.query.empty()) {
        path += "?" + url.query;
    }

    add(":method", request.method());
    add(":scheme", url.scheme);
    add(":path", path);
    add(":authority", build_authority(url));

    for (const auto& [key, value] : request.headers()) {
        if (key == "Host" || key == "Content-Length") {
            continue;
        }
        add(key, value);
    }
    return headers;
}

bool submit_request(SessionContext& ctx, const Request& request, int32_t& stream_id) {
    Url url = Url::parse(request.url());
    auto headers = build_headers(request, url);

    std::unique_ptr<BodyPayload> payload;
    nghttp2_data_provider2 data_provider{};
    if (!request.body().empty()) {
        payload = std::make_unique<BodyPayload>();
        payload->data = request.body();
        data_provider.source.ptr = payload.get();
        data_provider.read_callback = [](nghttp2_session*, int32_t, uint8_t* buf, size_t length,
                                          uint32_t* data_flags, nghttp2_data_source* source, void*)
                                          -> nghttp2_ssize {
            auto* body = static_cast<BodyPayload*>(source->ptr);
            if (!body || body->offset >= body->data.size()) {
                *data_flags = NGHTTP2_DATA_FLAG_EOF;
                return 0;
            }
            std::size_t remaining = body->data.size() - body->offset;
            std::size_t n = std::min(length, remaining);
            std::memcpy(buf, body->data.data() + body->offset, n);
            body->offset += n;
            if (body->offset >= body->data.size()) {
                *data_flags = NGHTTP2_DATA_FLAG_EOF;
            }
            return static_cast<nghttp2_ssize>(n);
        };
    }

    stream_id = nghttp2_submit_request2(ctx.session,
                                        nullptr,
                                        headers.data(),
                                        headers.size(),
                                        payload ? &data_provider : nullptr,
                                        nullptr);
    if (stream_id < 0) {
        return false;
    }

    ctx.responses[stream_id] = StreamResponse{};
    if (payload) {
        nghttp2_session_set_stream_user_data(ctx.session, stream_id, payload.release());
    }
    return true;
}

Response build_response(const StreamResponse& stream_response, const Url& url, const std::string& request_url) {
    Response response;
    response.set_status_code(stream_response.status_code);
    response.set_headers(stream_response.headers);
    response.set_text(stream_response.body);
    response.set_raw(stream_response.body);
    if (stream_response.error_code != ErrorCode::Unknown) {
        response.set_error_code(stream_response.error_code);
    }
    if (!stream_response.error_message.empty()) {
        response.set_text(stream_response.error_message);
    }
    response.set_error_url(request_url.empty() ? url.authority() : request_url);
    return response;
}

}  // namespace

Http2Adapter::Http2Adapter(Http2Config config) : impl_(std::make_unique<Impl>()) {
    impl_->config = std::move(config);
}

Http2Adapter::~Http2Adapter() = default;

Response Http2Adapter::send(const Request& request) {
    Url url = Url::parse(request.url());
    if (url.scheme != "https") {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::HTTP2);
        response.set_text("HTTP/2 requires https in this adapter");
        response.set_error_url(request.url());
        return response;
    }

    tls::TlsConfig tls_config{};
    tls_config.read_timeout_ms = request.timeout().read_ms;
    tls_config.write_timeout_ms = request.timeout().read_ms;
    tls_config.connect_timeout_ms = request.timeout().connect_ms;

    std::string tls_error;
    tls::TlsClient tls_client = impl_->tls_pool.acquire(url, tls_config, tls_error);
    if (!tls_client.is_open()) {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::TLS);
        response.set_text(tls_error);
        response.set_error_url(request.url());
        return response;
    }

    nghttp2_session_callbacks* callbacks = nullptr;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
    nghttp2_session_callbacks_set_on_frame_not_send_callback(
        callbacks,
        [](nghttp2_session*, const nghttp2_frame* frame, int lib_error_code, void* user_data) {
            auto* ctx = static_cast<SessionContext*>(user_data);
            if (!ctx) {
                return 0;
            }
            if (frame && frame->hd.stream_id != 0) {
                auto& resp = ctx->responses[frame->hd.stream_id];
                resp.error_code = ErrorCode::HTTP2;
                resp.error_message = nghttp2_strerror(lib_error_code);
            }
            return 0;
        });

    nghttp2_session* session = nullptr;
    nghttp2_option* option = nullptr;
    nghttp2_option_new(&option);
    nghttp2_option_set_peer_max_concurrent_streams(option, impl_->config.max_concurrent_streams);
    nghttp2_option_set_no_auto_window_update(option, 0);

    nghttp2_settings_entry settings[3] = {
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, impl_->config.max_concurrent_streams},
        {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, impl_->config.initial_stream_window_size},
        {NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, impl_->config.header_table_size}
    };

    nghttp2_session_client_new2(&session, callbacks, nullptr, option);
    nghttp2_submit_settings(session, NGHTTP2_FLAG_NONE, settings, 3);

    SessionContext ctx{};
    ctx.session = session;
    ctx.tls_client = std::move(tls_client);
    ctx.url = url;

    nghttp2_session_set_user_data(session, &ctx);

    static const std::string kPreface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    if (ctx.tls_client.send(kPreface) == 0) {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::IO);
        response.set_text("HTTP/2 preface send failed");
        response.set_error_url(request.url());
        nghttp2_session_callbacks_del(callbacks);
        nghttp2_session_del(session);
        nghttp2_option_del(option);
        return response;
    }

    int32_t stream_id = -1;
    if (!submit_request(ctx, request, stream_id)) {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::HTTP2);
        response.set_text("HTTP/2 submit request failed");
        response.set_error_url(request.url());
        nghttp2_session_callbacks_del(callbacks);
        nghttp2_session_del(session);
        nghttp2_option_del(option);
        return response;
    }

    nghttp2_session_send(session);
    if (!flush_send_buffer(ctx)) {
        Response response;
        response.set_status_code(0);
        response.set_error_code(ErrorCode::IO);
        response.set_text("HTTP/2 send failed");
        response.set_error_url(request.url());
        nghttp2_session_callbacks_del(callbacks);
        nghttp2_session_del(session);
        nghttp2_option_del(option);
        return response;
    }

    bool done = false;
    int loop_guard = 0;
    while (!done && loop_guard < 1000) {
        if (!recv_into_session(ctx)) {
            break;
        }
        if (nghttp2_session_want_read(session) == 0 && nghttp2_session_want_write(session) == 0) {
            done = true;
        }
        loop_guard++;
    }

    StreamResponse stream_response{};
    auto it = ctx.responses.find(stream_id);
    if (it != ctx.responses.end()) {
        stream_response = it->second;
    } else {
        stream_response.error_code = ErrorCode::HTTP2;
        stream_response.error_message = "HTTP/2 response missing";
    }

    Response response = build_response(stream_response, url, request.url());

    nghttp2_session_callbacks_del(callbacks);
    nghttp2_session_del(session);
    nghttp2_option_del(option);

    impl_->tls_pool.release(url, std::move(ctx.tls_client), true);
    return response;
}

Http2AdapterResult create_http2_adapter(Http2Config config) {
    Http2AdapterResult result{};
    result.adapter = std::make_shared<Http2Adapter>(std::move(config));
    result.fallback_to_http1 = false;
    result.error.clear();
    return result;
}

Http2AdapterResult create_http2_adapter() {
    return create_http2_adapter(Http2Config{});
}

}  // namespace requests_cpp::http2
