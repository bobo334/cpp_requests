#pragma once

// 跨平台符号导出宏定义
#ifdef _WIN32
    #ifdef REQUESTS_CPP_STATIC_DEFINE
        #define REQUESTS_CPP_API
    #elif defined(REQUESTS_CPP_EXPORTS)
        #define REQUESTS_CPP_API __declspec(dllexport)
    #else
        #define REQUESTS_CPP_API __declspec(dllimport)
    #endif
#else
    #ifdef REQUESTS_CPP_STATIC_DEFINE
        #define REQUESTS_CPP_API
    #elif defined(REQUESTS_CPP_EXPORTS)
        #define REQUESTS_CPP_API __attribute__((visibility("default")))
    #else
        #define REQUESTS_CPP_API
    #endif
#endif

// 为了向后兼容，也定义旧的宏
#ifndef REQUESTS_CPP_EXPORTS
    #ifdef _WIN32
        #ifdef REQUESTS_CPP_STATIC_DEFINE
            #define REQUESTS_CPP_EXPORT
        #else
            #define REQUESTS_CPP_EXPORT __declspec(dllimport)
        #endif
    #else
        #define REQUESTS_CPP_EXPORT
    #endif
#else
    #ifdef _WIN32
        #define REQUESTS_CPP_EXPORT __declspec(dllexport)
    #else
        #define REQUESTS_CPP_EXPORT __attribute__((visibility("default")))
    #endif
#endif
