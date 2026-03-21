from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
from conan.tools.build import check_min_cppstd
import os


class RequestsCppConan(ConanFile):
    name = "requests-cpp"
    version = "0.1.0"
    
    # Optional metadata
    license = "MIT"
    author = "requests-cpp contributors"
    url = "https://github.com/requests-cpp/requests-cpp"
    description = "A modern C++ HTTP library inspired by Python's requests"
    topics = ("http", "rest", "client", "requests")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_ssl": [True, False],
        "with_http2": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_ssl": True,
        "with_http2": True,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, "17")

    def requirements(self):
        self.requires("zlib/1.2.13")
        self.requires("brotli/1.0.9")
        if self.options.with_ssl:
            self.requires("openssl/1.1.1t")
        if self.options.with_http2:
            self.requires("nghttp2/1.52.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["REQUESTS_CPP_BUILD_EXAMPLES"] = False
        tc.variables["BUILD_SHARED_LIBS"] = self.options.shared
        tc.variables["REQUESTS_CPP_SSL_SUPPORT"] = self.options.with_ssl
        tc.variables["REQUESTS_CPP_HTTP2_SUPPORT"] = self.options.with_http2
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["requests_cpp"]
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs.extend(["pthread", "dl", "rt"])
        elif self.settings.os == "Windows":
            self.cpp_info.system_libs.append("ws2_32")