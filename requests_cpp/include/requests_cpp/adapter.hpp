#pragma once

#include <memory>
#include <string>

#include "requests_cpp/export.hpp"
#include "requests_cpp/request.hpp"
#include "requests_cpp/response.hpp"

namespace requests_cpp {

class Adapter {
public:
    virtual ~Adapter() = default;

    virtual Response send(const Request& request) = 0;
};

}  // namespace requests_cpp
