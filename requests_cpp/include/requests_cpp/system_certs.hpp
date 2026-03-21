#pragma once

#include <string>
#include <vector>

namespace requests_cpp::tls {

enum class SystemCertSource {
    None,
    Embedded,
    WindowsStore,
    LinuxSystem,
    MacosKeychain,
    CustomFile
};

struct SystemCertsResult {
    bool success{false};
    SystemCertSource source{SystemCertSource::None};
    std::string path;
    std::string error;
    std::vector<unsigned char> data;
};

SystemCertsResult load_system_certificates();

#if defined(_WIN32)
SystemCertsResult load_windows_store_certs();
#elif defined(__APPLE__)
SystemCertsResult load_macos_keychain_certs();
#elif defined(__linux__)
SystemCertsResult load_linux_system_certs();
#endif

}  // namespace requests_cpp::tls
