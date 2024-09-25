#ifndef FUNCTIONS_H
#define FUNCTIONS_H


#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <openssl/rand.h>
#include "crow_all.h"
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iomanip>

std::string parse_cookie(const std::string& cookie_string, const std::string& key) {
    std::istringstream cookie_stream(cookie_string);
    std::string token;
    while (std::getline(cookie_stream, token, ';')) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string token_key = token.substr(0, pos);
            std::string token_value = token.substr(pos + 1);
            // Trim leading and trailing whitespace
            token_key.erase(0, token_key.find_first_not_of(" "));
            token_key.erase(token_key.find_last_not_of(" ") + 1);
            token_value.erase(0, token_value.find_first_not_of(" "));
            token_value.erase(token_value.find_last_not_of(" ") + 1);
            if (token_key == key) {
                return token_value;
            }
        }
    }
    return "";
}

std::string hash_password(const std::string& password) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Error creating EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error initializing digest");
    }

    if (EVP_DigestUpdate(mdctx, password.c_str(), password.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error updating digest");
    }

    if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error finalizing digest");
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}



// Generate a secure random token
std::string generate_random_token(size_t length = 32) {
    const char charset[] = "0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, max_index);

    std::string token;
    token.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        token += charset[distribution(generator)];
    }

    return token;
}

// Hash a token using SHA-256
std::string hash_token(const std::string& token) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, token.c_str(), token.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}


std::string json_value_to_string(const crow::json::rvalue& value) {
    if (value.t() == crow::json::type::String) {
        return value.s();
    } else {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }
}

void insert(pqxx::work& W, const std::string& tableName, const crow::json::rvalue& columns) {
    std::string query = "INSERT INTO " + tableName + " (";
    std::string values = "VALUES (";
    for (const auto& key : columns.keys()) {
        query += key + ", ";
        values += W.quote(json_value_to_string(columns[key])) + ", ";
    }
    query = query.substr(0, query.length() - 2) + ") ";
    values = values.substr(0, values.length() - 2) + ");";
    W.exec(query + values);
}



void user_update(pqxx::work& W, const std::string& tableName, const std::string& id, const crow::json::rvalue& columns) {
    std::string query = "UPDATE " + tableName + " SET ";
    for (const auto& key : columns.keys()) {
        if (key != "user_id") {
        if(key == "password"){
            string x = "'" +hash_password(W.quote(json_value_to_string(columns[key])))+"'";
             query += key + " = " + x + ", ";
             continue;
        }
            query += key + " = " + W.quote(json_value_to_string(columns[key])) + ", ";
        }
    }
    query = query.substr(0, query.length() - 2);
    query += " WHERE user_id = " + id + ";";
    W.exec(query);
}

void service_update(pqxx::work& W, const std::string& tableName, const std::string& id, const crow::json::rvalue& columns) {
    std::string query = "UPDATE " + tableName + " SET ";
    for (const auto& key : columns.keys()) {
        if (key != "service_id") {
            query += key + " = " + W.quote(json_value_to_string(columns[key])) + ", ";
        }
    }
    query = query.substr(0, query.length() - 2);
    query += " WHERE service_id = " + id + ";";
    W.exec(query);
}


void authority_update(pqxx::work& W, const std::string& tableName, const std::string& id, const crow::json::rvalue& columns) {
    std::string query = "UPDATE " + tableName + " SET ";
    for (const auto& key : columns.keys()) {
        if (key != "authority_id") {
            query += key + " = " + W.quote(json_value_to_string(columns[key])) + ", ";
        }
    }
    query = query.substr(0, query.length() - 2);
    query += " WHERE authority_id = " + id + ";";
    W.exec(query);
}



void update(pqxx::work& W, const std::string& tableName, const std::string& id, const crow::json::rvalue& columns) {
    std::string query = "UPDATE " + tableName + " SET ";
    for (const auto& key : columns.keys()) {
        if (key != "id") {
            query += key + " = " + W.quote(json_value_to_string(columns[key])) + ", ";
        }
    }
    query = query.substr(0, query.length() - 2);
    query += " WHERE id = " + id + ";";
    W.exec(query);
}


void user_delete_record(pqxx::work& W, const std::string& tableName, const std::string& id) {
    std::string query = "DELETE FROM " + tableName + " WHERE user_id = " + id + ";";
    W.exec(query);
}

void service_delete_record(pqxx::work& W, const std::string& tableName, const std::string& id) {
    std::string query = "DELETE FROM " + tableName + " WHERE service_id = " + id + ";";
    W.exec(query);
}

void authority_delete_record(pqxx::work& W, const std::string& tableName, const std::string& id) {
    std::string query = "DELETE FROM " + tableName + " WHERE authority_id = " + id + ";";
    W.exec(query);
}


void delete_record(pqxx::work& W, const std::string& tableName, const std::string& id) {
    std::string query = "DELETE FROM " + tableName + " WHERE id = " + id + ";";
    W.exec(query);
}



#endif
