cmake_minimum_required(VERSION 3.16)

project(requests_cpp VERSION 0.1.0 LANGUAGES C CXX)

if (WIN32)
    enable_language(ASM_MASM)
endif()

include(GNUInstallDirs)

option(REQUESTS_CPP_BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(REQUESTS_CPP_SOURCES
    src/adapter.cpp
    src/connection_pool.cpp
    src/exceptions.cpp
    src/http1.cpp
    src/http1_adapter.cpp
    src/http2_adapter.cpp
    src/http2_smoke.cpp
    src/request.cpp
    src/response.cpp
    src/session.cpp
    src/system_certs.cpp
    src/tls_connection_pool.cpp
    src/url.cpp
    src/api.cpp
    src/proxy.cpp
    src/proxy_tunnel.cpp
    src/auth.cpp
    src/cookie.cpp
    src/retry.cpp
    src/md5.cpp
    src/ntlm.cpp
    src/tls_enhanced.cpp
    src/streaming.cpp
    third_party/miniz/miniz.c
    third_party/miniz/miniz_tinfl.c
    third_party/miniz/miniz_tdef.c
    third_party/brotli/common/constants.c
    third_party/brotli/common/context.c
    third_party/brotli/common/dictionary.c
    third_party/brotli/common/platform.c
    third_party/brotli/common/shared_dictionary.c
    third_party/brotli/common/transform.c
    third_party/brotli/dec/bit_reader.c
    third_party/brotli/dec/decode.c
    third_party/brotli/dec/huffman.c
    third_party/brotli/dec/prefix.c
    third_party/brotli/dec/state.c
    third_party/brotli/dec/static_init.c
    third_party/nghttp2/lib/nghttp2_alpn.c
    third_party/nghttp2/lib/nghttp2_buf.c
    third_party/nghttp2/lib/nghttp2_callbacks.c
    third_party/nghttp2/lib/nghttp2_debug.c
    third_party/nghttp2/lib/nghttp2_extpri.c
    third_party/nghttp2/lib/nghttp2_frame.c
    third_party/nghttp2/lib/nghttp2_hd.c
    third_party/nghttp2/lib/nghttp2_hd_huffman.c
    third_party/nghttp2/lib/nghttp2_hd_huffman_data.c
    third_party/nghttp2/lib/nghttp2_helper.c
    third_party/nghttp2/lib/nghttp2_http.c
    third_party/nghttp2/lib/nghttp2_map.c
    third_party/nghttp2/lib/nghttp2_mem.c
    third_party/nghttp2/lib/nghttp2_option.c
    third_party/nghttp2/lib/nghttp2_outbound_item.c
    third_party/nghttp2/lib/nghttp2_pq.c
    third_party/nghttp2/lib/nghttp2_priority_spec.c
    third_party/nghttp2/lib/nghttp2_queue.c
    third_party/nghttp2/lib/nghttp2_ratelim.c
    third_party/nghttp2/lib/nghttp2_rcbuf.c
    third_party/nghttp2/lib/nghttp2_session.c
    third_party/nghttp2/lib/nghttp2_stream.c
    third_party/nghttp2/lib/nghttp2_submit.c
    third_party/nghttp2/lib/nghttp2_time.c
    third_party/nghttp2/lib/nghttp2_version.c
    third_party/nghttp2/lib/sfparse.c
)

if (APPLE)
    list(APPEND REQUESTS_CPP_SOURCES src/tls_macos.cpp)
elseif (WIN32)
    list(APPEND REQUESTS_CPP_SOURCES src/tls_win.cpp)
elseif (UNIX)
    list(APPEND REQUESTS_CPP_SOURCES src/tls_linux.cpp)
else()
    list(APPEND REQUESTS_CPP_SOURCES src/tls_stub.cpp)
endif()

set(TINYTLS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TinyTls)
file(GLOB_RECURSE TINYTLS_SOURCES
    ${TINYTLS_ROOT}/src/*.c
    ${TINYTLS_ROOT}/src/*.cpp
)

list(REMOVE_ITEM TINYTLS_SOURCES
    ${TINYTLS_ROOT}/src/net/TcpSockLinux.cpp
    ${TINYTLS_ROOT}/src/net/TcpSockWin32.cpp
    ${TINYTLS_ROOT}/src/ecc/ecp_nistp256.c
)

set(TINYTLS_ASM_SOURCES)
if (WIN32)
    # 禁用 MASM 路径以避免 Windows 构建中的 .asm 输出路径错误
else()
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64")
        list(APPEND TINYTLS_ASM_SOURCES ${TINYTLS_ROOT}/src/x86_64/platform.s)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86")
        list(APPEND TINYTLS_ASM_SOURCES ${TINYTLS_ROOT}/src/i386/platform.s)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
        list(APPEND TINYTLS_ASM_SOURCES ${TINYTLS_ROOT}/src/arm64/platform.s)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        list(APPEND TINYTLS_ASM_SOURCES ${TINYTLS_ROOT}/src/arm/platform.s)
    endif()
endif()

# Testing support
option(REQUESTS_CPP_BUILD_TESTS "Build test programs" ON)

if(REQUESTS_CPP_BUILD_TESTS)
    enable_testing()
    
    add_executable(unit_tests tests/unit_tests.cpp)
    target_link_libraries(unit_tests PRIVATE requests_cpp)
    set_target_properties(unit_tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )
    
    add_executable(performance_tests tests/performance_tests.cpp)
    target_link_libraries(performance_tests PRIVATE requests_cpp)
    set_target_properties(performance_tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )
    
    add_test(NAME unit_tests COMMAND unit_tests)
endif()

list(APPEND REQUESTS_CPP_SOURCES ${TINYTLS_SOURCES} ${TINYTLS_ASM_SOURCES})

if(BUILD_SHARED_LIBS)
    add_library(requests_cpp SHARED ${REQUESTS_CPP_SOURCES})
    set_target_properties(requests_cpp PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    target_compile_definitions(requests_cpp
        PRIVATE
            REQUESTS_CPP_EXPORTS
            BUILDING_NGHTTP2
            HAVE_CONFIG_H
    )
else()
    add_library(requests_cpp STATIC ${REQUESTS_CPP_SOURCES})
    set_target_properties(requests_cpp PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        VERSION ${PROJECT_VERSION}
    )
    target_compile_definitions(requests_cpp
        PUBLIC
            REQUESTS_CPP_STATIC_DEFINE
        PRIVATE
            BUILDING_NGHTTP2
            HAVE_CONFIG_H
    )
endif()

target_include_directories(requests_cpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/ssl>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/net>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/cert>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/sha>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/ecc>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/rsa>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/cha>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/include/aes>
        $<BUILD_INTERFACE:${TINYTLS_ROOT}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/dec>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/brotli/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/nghttp2/lib/includes>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/nghttp2/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
 )

target_compile_definitions(requests_cpp
    PRIVATE
        BUILDING_NGHTTP2
        HAVE_CONFIG_H
)

if (WIN32)
    target_link_libraries(requests_cpp PRIVATE ws2_32)
endif()

install(TARGETS requests_cpp
    EXPORT requests_cppTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT requests_cppTargets
    FILE requests_cppTargets.cmake
    NAMESPACE requests_cpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/requests_cpp
)

if (REQUESTS_CPP_BUILD_EXAMPLES)
    add_executable(decode_chain_smoke examples/decode_chain_smoke.cpp)
    target_link_libraries(decode_chain_smoke PRIVATE requests_cpp)
    set_target_properties(decode_chain_smoke PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    add_executable(http1_chunked_trailers_decode examples/http1_chunked_trailers_decode.cpp)
    target_link_libraries(http1_chunked_trailers_decode PRIVATE requests_cpp)
    set_target_properties(http1_chunked_trailers_decode PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    add_executable(http2_smoke examples/http2_smoke.cpp)
    target_link_libraries(http2_smoke PRIVATE requests_cpp)
    set_target_properties(http2_smoke PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )
    
    add_executable(advanced_features_example examples/advanced_features_example.cpp)
    target_link_libraries(advanced_features_example PRIVATE requests_cpp)
    set_target_properties(advanced_features_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    add_executable(proxy_tunnel_example examples/proxy_tunnel_example.cpp)
    target_link_libraries(proxy_tunnel_example PRIVATE requests_cpp)
    set_target_properties(proxy_tunnel_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    add_executable(advanced_auth_proxy_example examples/advanced_auth_proxy_example.cpp)
    target_link_libraries(advanced_auth_proxy_example PRIVATE requests_cpp)
    set_target_properties(advanced_auth_proxy_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )

    add_executable(tls_enhanced_example examples/tls_enhanced_example.cpp)
    target_link_libraries(tls_enhanced_example PRIVATE requests_cpp)
    set_target_properties(tls_enhanced_example PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
    )
    
endif()
