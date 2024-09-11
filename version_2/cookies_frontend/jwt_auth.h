#ifndef JWT_AUTH_H
#define JWT_AUTH_H

#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>
#include "crow_all.h"
#include <pqxx/pqxx>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>

class JWTAuth {
private:
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

    std::pair<bool, std::string> refresh_token_if_needed(const std::string& token) {
        try {
            auto decoded = jwt::decode(token);
            auto exp = decoded.get_expires_at();
            if (std::chrono::system_clock::now() + std::chrono::minutes(5) >= exp) {
                // Token will expire soon, refresh it
                std::string username = decoded.get_payload_claim("username").as_string();
                std::string new_token = create_jwt(username);
                return {true, new_token};
            }
        } catch (const std::exception& e) {
            std::cerr << "Token refresh error: " << e.what() << std::endl;
        }
        return {false, ""};
    }
    
    std::mutex token_mutex;
    std::unordered_map<std::string, std::string> user_tokens;

void save_token_to_file(const std::string& username, const std::string& token) {
    std::lock_guard<std::mutex> lock(token_mutex);
    std::ofstream file(username + "_token.txt");
    if (!file) {
        throw std::runtime_error("Unable to open token file for writing");
    }
    file << token;
}

std::string load_token_from_file(const std::string& username) {
    std::lock_guard<std::mutex> lock(token_mutex);
    std::ifstream file( username + "_token.txt");
    if (!file) {
        throw std::runtime_error("Unable to open token file for reading");
    }
    std::string token;
    std::getline(file, token);
    return token;
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

        // Assume we are checking the password against a database
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        pqxx::result r = W.exec_params("SELECT password FROM users WHERE username = $1", username);

        if (r.size() != 1) {
            // Username not found
            return crow::response(401, "Invalid username or password");
        }

        std::string db_password = r[0]["password"].as<std::string>();

        // Check if the provided password matches the stored password
        if (password == db_password) {
            W.commit();
            
            std::string token = create_jwt(username);
            save_token_to_file(username, token);
            user_tokens[username] = token;

            crow::response res(200, "Login successful");
            // removed the HttpOnly that caused the CORS error
            res.add_header("Set-Cookie", "jwt=" + token + "; Path=/; Max-Age=" + std::to_string(TOKEN_EXPIRY_SECONDS));
            
            return res;
        } else {
            return crow::response(401, "Invalid username or password");
        }
    }
        bool check_and_refresh_token(const std::string& username, const std::string& client_token) {
        std::cout << client_token << '\n';
        //std::lock_guard<std::mutex> lock(token_mutex);
        std::string server_token = load_token_from_file(username);
        std::cout << server_token << '\n';
        if (client_token == server_token) {
            std::string new_token = create_jwt(username);
            save_token_to_file(username, new_token);
            return true;
        }
        return false;
    }

    std::string get_new_token(const std::string& username) {
        
        return load_token_from_file(username);
    }
    
    

    
};

#endif // JWT_AUTH_H