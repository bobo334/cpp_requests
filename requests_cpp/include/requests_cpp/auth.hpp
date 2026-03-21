#pragma once

#include <string>
#include <functional>

#include "requests_cpp/export.hpp"

namespace requests_cpp {

// Base class for authentication
class REQUESTS_CPP_API Auth {
public:
    virtual ~Auth() = default;
    virtual std::string get_auth_header() const = 0;
};

// Basic Authentication implementation
class BasicAuth : public Auth {
public:
    BasicAuth(const std::string& username, const std::string& password);
    
    std::string get_auth_header() const override;
    
private:
    std::string username_;
    std::string password_;
};

// Bearer Token Authentication implementation
class BearerTokenAuth : public Auth {
public:
    explicit BearerTokenAuth(const std::string& token);
    
    std::string get_auth_header() const override;
    
private:
    std::string token_;
};

// Digest Authentication implementation
class DigestAuth : public Auth {
public:
    DigestAuth(const std::string& username, const std::string& password);
    
    std::string get_auth_header() const override;
    
    // Process challenge from server
    void process_challenge(const std::string& challenge);
    
private:
    std::string username_;
    std::string password_;
    
    // Digest auth parameters
    std::string realm_;
    std::string nonce_;
    std::string algorithm_;
    std::string qop_;
    int nc_{1};  // Nonce count
    std::string cnonce_;  // Client nonce
    
    // Generate client nonce
    std::string generate_cnonce() const;
    
    // Calculate digest response
    std::string calculate_digest_response(
        const std::string& method,
        const std::string& uri,
        const std::string& entity_body = "") const;
};

// NTLM Authentication implementation
class NtlmAuth : public Auth {
public:
    NtlmAuth(const std::string& username, const std::string& password, 
             const std::string& domain = "", const std::string& workstation = "");
    
    std::string get_auth_header() const override;
    
    // Process NTLM handshake challenges
    void process_challenge(const std::string& challenge);
    
private:
    std::string username_;
    std::string password_;
    std::string domain_;
    std::string workstation_;
    
    // NTLM state
    bool has_negotiated_{false};
    
    // Generate NTLM type 1 message
    std::string generate_type1_message() const;
    
    // Generate NTLM type 3 message from type 2 challenge
    std::string generate_type3_message(const std::string& type2_msg) const;
};

}  // namespace requests_cpp