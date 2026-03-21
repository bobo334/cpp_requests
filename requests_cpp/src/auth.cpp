#include "requests_cpp/auth.hpp"
#include "requests_cpp/md5.hpp"
#include "requests_cpp/ntlm.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Include base64 encoding function
#include <vector>
#include <string>

namespace requests_cpp {

// Base64 encoding helper
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline std::string base64_encode(unsigned char const* buf, unsigned int bufLen) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char array_3[3];
    unsigned char array_4[4];

    while (bufLen--) {
        array_3[i++] = *(buf++);
        if (i == 3) {
            array_4[0] = (array_3[0] & 0xfc) >> 2;
            array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
            array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
            array_4[3] = array_3[2] & 0x3f;

            for(i = 0; (i < 4) ; i++)
                ret += base64_chars[array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            array_3[j] = '\0';

        array_4[0] = (array_3[0] & 0xfc) >> 2;
        array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
        array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
        array_4[3] = array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

BasicAuth::BasicAuth(const std::string& username, const std::string& password) 
    : username_(username), password_(password) {}

std::string BasicAuth::get_auth_header() const {
    std::string credentials = username_ + ":" + password_;
    std::vector<unsigned char> cred_bytes(credentials.begin(), credentials.end());
    return "Basic " + base64_encode(cred_bytes.data(), cred_bytes.size());
}

BearerTokenAuth::BearerTokenAuth(const std::string& token) : token_(token) {}

std::string BearerTokenAuth::get_auth_header() const {
    return "Bearer " + token_;
}

DigestAuth::DigestAuth(const std::string& username, const std::string& password) 
    : username_(username), password_(password) {}

std::string DigestAuth::get_auth_header() const {
    if (realm_.empty() || nonce_.empty()) {
        // If we don't have challenge parameters, we can't create a valid digest header
        return "";
    }
    
    std::string response = calculate_digest_response("GET", "/");
    
    std::ostringstream oss;
    oss << "Digest username=\"" << username_ << "\", "
        << "realm=\"" << realm_ << "\", "
        << "nonce=\"" << nonce_ << "\", "
        << "uri=\"/\", "
        << "algorithm=" << (algorithm_.empty() ? "MD5" : algorithm_) << ", "
        << "response=\"" << response << "\"";
    
    if (!qop_.empty()) {
        oss << ", qop=" << qop_ << ", nc=" << std::setfill('0') << std::setw(8) << nc_ << ", cnonce=\"" << cnonce_ << "\"";
    }
    
    return oss.str();
}

// Improved WWW-Authenticate header parser
void DigestAuth::process_challenge(const std::string& challenge) {
    // Parse the WWW-Authenticate header to extract digest parameters
    std::string auth_header = challenge;
    if (auth_header.substr(0, 6) == "Digest" || auth_header.substr(0, 6) == "digest") {
        auth_header = auth_header.substr(6);  // Remove "Digest" prefix
        // Remove leading space if present
        if (!auth_header.empty() && auth_header[0] == ' ') {
            auth_header = auth_header.substr(1);
        }
        
        size_t pos = 0;
        while (pos < auth_header.length()) {
            // Skip leading whitespace
            while (pos < auth_header.length() && (auth_header[pos] == ' ' || auth_header[pos] == '\t')) {
                pos++;
            }
            
            if (pos >= auth_header.length()) break;
            
            // Find key-value pair
            size_t eq_pos = auth_header.find('=', pos);
            if (eq_pos == std::string::npos) break;
            
            std::string key = auth_header.substr(pos, eq_pos - pos);
            // Trim whitespace from key
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            
            pos = eq_pos + 1;
            if (pos >= auth_header.length()) break;
            
            // Skip whitespace after equals
            while (pos < auth_header.length() && (auth_header[pos] == ' ' || auth_header[pos] == '\t')) {
                pos++;
            }
            
            if (pos >= auth_header.length()) break;
            
            std::string value;
            if (auth_header[pos] == '"') {
                // Quoted value
                pos++; // Skip opening quote
                size_t end_quote = auth_header.find('"', pos);
                if (end_quote != std::string::npos) {
                    value = auth_header.substr(pos, end_quote - pos);
                    pos = end_quote + 1; // Move past closing quote
                    
                    // Skip comma and any following whitespace
                    if (pos < auth_header.length() && auth_header[pos] == ',') {
                        pos++;
                        while (pos < auth_header.length() && (auth_header[pos] == ' ' || auth_header[pos] == '\t')) {
                            pos++;
                        }
                    }
                } else {
                    break;  // Malformed
                }
            } else {
                // Unquoted value
                size_t end_pos = auth_header.find(',', pos);
                if (end_pos == std::string::npos) {
                    end_pos = auth_header.length();
                }
                
                value = auth_header.substr(pos, end_pos - pos);
                // Trim whitespace from value
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                pos = end_pos;
                if (pos < auth_header.length()) { // Skip comma
                    pos++;
                }
            }
            
            // Store the parsed parameter
            if (key == "realm") {
                realm_ = value;
            } else if (key == "nonce") {
                nonce_ = value;
            } else if (key == "algorithm") {
                algorithm_ = value;
            } else if (key == "qop") {
                qop_ = value;
            } else if (key == "opaque") {
                // Store opaque if needed
            } else if (key == "stale") {
                // Handle stale parameter if needed
            } else if (key == "domain") {
                // Handle domain parameter if needed
            }
        }
    }
    
    // Generate a new client nonce for this request
    cnonce_ = generate_cnonce();
}

std::string DigestAuth::generate_cnonce() const {
    // Generate a random hex string as client nonce
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::string cnonce;
    for (int i = 0; i < 16; ++i) {
        int val = dis(gen);
        cnonce += (val < 10) ? ('0' + val) : ('a' + val - 10);
    }
    
    return cnonce;
}

std::string DigestAuth::calculate_digest_response(
    const std::string& method,
    const std::string& uri,
    const std::string& entity_body) const {
    
    // Calculate HA1: MD5(username:realm:password)
    std::string ha1_input = username_ + ":" + realm_ + ":" + password_;
    std::string ha1 = md5_hash(ha1_input);
    
    // Calculate HA2: MD5(method:digestURI)
    std::string ha2_input = method + ":" + uri;
    std::string ha2 = md5_hash(ha2_input);
    
    // Calculate response: MD5(HA1:nonce:HA2) or MD5(HA1:nonce:nc:cnonce:qop:HA2) if qop is specified
    std::string response_input;
    if (!qop_.empty()) {
        response_input = ha1 + ":" + nonce_ + ":" + std::to_string(nc_) + ":" + cnonce_ + ":" + qop_ + ":" + ha2;
    } else {
        response_input = ha1 + ":" + nonce_ + ":" + ha2;
    }
    
    return md5_hash(response_input);
}

NtlmAuth::NtlmAuth(const std::string& username, const std::string& password, 
                   const std::string& domain, const std::string& workstation)
    : username_(username), password_(password), domain_(domain), workstation_(workstation) {}

std::string NtlmAuth::get_auth_header() const {
    if (!has_negotiated_) {
        // Send Type 1 message for initial negotiation
        return "NTLM " + generate_type1_message();
    } else {
        // This would normally happen after receiving a Type 2 challenge
        // We'll return an empty header since we don't have the server challenge here
        // In a real implementation, you'd store the Type 2 challenge and generate Type 3
        return "";
    }
}

void NtlmAuth::process_challenge(const std::string& challenge) {
    // Check if this is a Type 2 NTLM challenge
    if (challenge.substr(0, 5) == "NTLM ") {
        std::string type2_msg = challenge.substr(5);
        // Generate Type 3 response
        // In a real implementation, you'd store the Type 2 message to generate the Type 3 response
        has_negotiated_ = true;
    }
}

std::string NtlmAuth::generate_type1_message() const {
    return create_ntlm_type1(domain_, workstation_);
}

std::string NtlmAuth::generate_type3_message(const std::string& type2_msg) const {
    // In a real implementation, we would extract the server challenge and target info
    // from the Type 2 message. For this implementation, we'll use placeholder values.
    // A complete implementation would properly parse the Type 2 message.
    return create_ntlm_type3(username_, domain_, workstation_, password_, "server_chlg", "target_info");
}

}  // namespace requests_cpp