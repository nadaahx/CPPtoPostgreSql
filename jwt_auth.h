#ifndef JWT_AUTH_H
#define JWT_AUTH_H

#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>
#include "crow_all.h"

class JWTAuth {
private:
    const std::string TOKEN_FILE = "jwt_token.txt";
    const std::string SECURE_KEY_FILE = "secure_key.bin";
    const int KEY_LENGTH = 32;
    const int TOKEN_EXPIRY_SECONDS = 3600;
    
    std::string secure_key;

    std::string generate_secure_key(int length) {
        std::vector<unsigned char> buffer(length);
        if (RAND_bytes(buffer.data(), length) != 1) {
            throw std::runtime_error("Failed to generate secure key");
        }
        return std::string(buffer.begin(), buffer.end());
    }

    void load_or_generate_secure_key() {
        std::ifstream key_file(SECURE_KEY_FILE, std::ios::binary);
        if (key_file.good()) {
            secure_key = std::string((std::istreambuf_iterator<char>(key_file)),
                                     std::istreambuf_iterator<char>());
        } else {
            secure_key = generate_secure_key(KEY_LENGTH);
            std::ofstream out_file(SECURE_KEY_FILE, std::ios::binary);
            out_file.write(secure_key.c_str(), secure_key.length());
        }
    }

    std::string create_jwt(const std::string& username) {
        auto token = jwt::create()
            .set_issuer("auth_server")
            .set_subject(username)
            .set_payload_claim("username", jwt::claim(username))
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{TOKEN_EXPIRY_SECONDS})
            .sign(jwt::algorithm::hs256{secure_key});
        
        // Save token to file
        std::ofstream token_file(TOKEN_FILE);
        token_file << token;
        
        return token;
    }

    bool validate_jwt(const std::string& token) {
        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{secure_key})
                .with_issuer("auth_server");
            verifier.verify(decoded);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Token validation error: " << e.what() << std::endl;
            return false;
        }
    }

    std::string get_stored_token() {
        std::ifstream token_file(TOKEN_FILE);
        if (token_file.good()) {
            std::string token;
            std::getline(token_file, token);
            return token;
        }
        return "";
    }

    void refresh_token_if_needed() {
        std::string token = get_stored_token();
        if (!token.empty()) {
            try {
                auto decoded = jwt::decode(token);
                auto exp = decoded.get_expires_at();
                if (std::chrono::system_clock::now() + std::chrono::minutes(5) >= exp) {
                    // Token will expire soon, refresh it
                    std::string username = decoded.get_payload_claim("username").as_string();
                    create_jwt(username);
                }
            } catch (const std::exception& e) {
                std::cerr << "Token refresh error: " << e.what() << std::endl;
            }
        }
    }

public:
    JWTAuth() {
        load_or_generate_secure_key();
    }

    crow::response login(const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string username = x["username"].s();
        std::string password = x["password"].s();

        // Here you should implement proper password checking
        if (password == "secret_password") {  // This is just an example
            std::string token = create_jwt(username);
            return crow::response(200, token);
        } else {
            return crow::response(401, "Invalid credentials");
        }
    }

    bool protect_route(const crow::request& req) {
        refresh_token_if_needed();
        
        auto auth_header = req.get_header_value("Authorization");
        if (auth_header.substr(0, 7) == "Bearer ") {
            std::string token = auth_header.substr(7);
            if (validate_jwt(token)) {
                return true; // Token is valid
            }
        }
        return false;
    }
};

#endif // JWT_AUTH_H
