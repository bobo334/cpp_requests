#include "requests_cpp/system_certs.hpp"

#include <fstream>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#elif defined(__APPLE__)
#include <Security/Security.h>
#include <Security/SecCertificate.h>
#endif

namespace requests_cpp::tls {

namespace {
bool read_file_bytes(const std::string& path, std::vector<unsigned char>& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<std::size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.good();
}
}  // namespace

SystemCertsResult load_system_certificates() {
    SystemCertsResult result;

#if defined(_WIN32)
    return load_windows_store_certs();
#elif defined(__APPLE__)
    return load_macos_keychain_certs();
#elif defined(__linux__)
    return load_linux_system_certs();
#else
    result.error = "System certificate loading not supported on this platform";
    return result;
#endif
}

#if defined(_WIN32)

SystemCertsResult load_windows_store_certs() {
    SystemCertsResult result;
    result.source = SystemCertSource::WindowsStore;

    HCERTSTORE hStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        0,
        0,
        CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG,
        L"ROOT"
    );

    if (!hStore) {
        hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM,
            0,
            0,
            CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
            L"ROOT"
        );
    }

    if (!hStore) {
        result.error = "Failed to open Windows certificate store";
        return result;
    }

    std::vector<unsigned char> allCerts;
    int certCount = 0;
    PCCERT_CONTEXT pContext = nullptr;

    while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != nullptr) {
        if (pContext->pbCertEncoded && pContext->cbCertEncoded > 0) {
            allCerts.insert(
                allCerts.end(),
                pContext->pbCertEncoded,
                pContext->pbCertEncoded + pContext->cbCertEncoded
            );
            ++certCount;
        }
    }

    CertCloseStore(hStore, 0);

    if (certCount > 0) {
        result.success = true;
        result.data = std::move(allCerts);
    } else {
        result.error = "No certificates found in Windows Store";
    }

    return result;
}

#elif defined(__APPLE__)

SystemCertsResult load_macos_keychain_certs() {
    SystemCertsResult result;
    result.source = SystemCertSource::MacosKeychain;

    CFArrayRef certs = nullptr;
    OSStatus status = SecTrustCopyAnchorCertificates(&certs);

    if (status != errSecSuccess || !certs) {
        result.error = "Failed to access macOS keychain";
        return result;
    }

    CFIndex count = CFArrayGetCount(certs);
    std::vector<unsigned char> allCerts;
    int certCount = 0;

    for (CFIndex i = 0; i < count; ++i) {
        SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex(certs, i);
        if (!cert) {
            continue;
        }

        CFDataRef data = SecCertificateCopyData(cert);
        if (!data) {
            continue;
        }

        const UInt8* bytes = CFDataGetBytePtr(data);
        CFIndex length = CFDataGetLength(data);

        if (bytes && length > 0) {
            allCerts.insert(allCerts.end(), bytes, bytes + length);
            ++certCount;
        }

        CFRelease(data);
    }

    CFRelease(certs);

    if (certCount > 0) {
        result.success = true;
        result.data = std::move(allCerts);
    } else {
        result.error = "No certificates found in macOS Keychain";
    }

    return result;
}

#elif defined(__linux__)

SystemCertsResult load_linux_system_certs() {
    SystemCertsResult result;
    result.source = SystemCertSource::LinuxSystem;

    const char* certPaths[] = {
        "/etc/ssl/certs/ca-certificates.crt",
        "/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/ca-bundle.pem",
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
        "/etc/pki/tls/certs/ca-bundle.trust.crt",
        nullptr
    };

    for (int i = 0; certPaths[i] != nullptr; ++i) {
        std::vector<unsigned char> data;
        if (read_file_bytes(certPaths[i], data)) {
            result.success = true;
            result.path = certPaths[i];
            result.data = std::move(data);
            return result;
        }
    }

    result.error = "No system certificate bundle found";
    return result;
}

#endif

}  // namespace requests_cpp::tls
