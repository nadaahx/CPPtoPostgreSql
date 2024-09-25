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

using namespace std;

/* g++ main.cpp -o output -lpqxx -lpq  -lssl -lcrypto */


// Simple MIME type mapping
std::unordered_map<std::string, std::string> mime_types = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"}
};


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

/*

// Function to hash a password using SHA-256
std::string hash_password(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}*/


#include <openssl/evp.h>
#include <iomanip>

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




int main() {
    try {


    crow::SimpleApp app;
 

CROW_ROUTE(app, "/")([]() {
    return "Hello, World!";
});

CROW_ROUTE(app, "/update-user").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");
    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string username = data["username"].s();
        auto columns = data["columns"];
        
        // Prepare the update statement dynamically based on provided columns
        std::string update_query = "UPDATE Users SET ";
        std::vector<std::string> params;
        int index = 1;
        if (columns.has("email")) {
            update_query += "email=$" + std::to_string(index++) + ", ";
            params.push_back(columns["email"].s());
        }
        if (columns.has("phone_number")) {
            update_query += "phone_number=$" + std::to_string(index++) + ", ";
            params.push_back(columns["phone_number"].s());
        }
        if (columns.has("address")) {
            update_query += "address=$" + std::to_string(index++) + ", ";
            params.push_back(columns["address"].s());
        }
        if (index > 1) {
            // Remove trailing comma and space
            update_query = update_query.substr(0, update_query.size() - 2);
            update_query += " WHERE username=$" + std::to_string(index);
            params.push_back(username);

            // Execute the query with individual parameters
            pqxx::result r = W.exec_params(
                update_query,
                params[0],
                params[1],
                params[2],
                params[3]  // Add or remove parameters as needed
            );
            
            W.commit();
            return crow::response(200, "User updated successfully");
        } else {
            return crow::response(400, "No valid columns provided for update");
        }
    } catch (const std::exception& e) {
        return crow::response(500, std::string("Error: ") + e.what());
    }
});


CROW_ROUTE(app, "/insert").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {  
    try {
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        std::string tableName = data["table_name"].s();
        auto columns = data["columns"];

        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        insert(W, tableName, columns);
        W.commit();
        return crow::response(200, "Inserted into table successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/read_all").methods(crow::HTTPMethod::GET)
([](const crow::request& req) { 
    try {
        auto tableName = req.url_params.get("table");
        if (!tableName) return crow::response(400, "Table name is required");
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        pqxx::result R;
        std::string x = W.esc(tableName);
        
        // curl -X GET "http://38.242.215.0:18080/read_all?table=authorities_requests"
        
        if(x=="authorities_requests"){
           R = W.exec("SELECT * FROM Authorities WHERE is_request=TRUE;");
        }
        else R = W.exec("SELECT * FROM " + W.esc(tableName) + ";");

        crow::json::wvalue result;
        for (size_t i = 0; i < R.size(); ++i) {
            crow::json::wvalue json_row;
            for (const auto& field : R[i]) {
                if (field.is_null()) {
                    json_row[field.name()] = nullptr;
                } else {
                    json_row[field.name()] = field.as<std::string>();
                }
            }
            result[i] = std::move(json_row);
        }
        return crow::response(200, result);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/update").methods(crow::HTTPMethod::POST)
([](const crow::request& req) { 
    try {
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        std::string tableName = data["table_name"].s();
        std::string id = data["columns"]["id"].s();
        auto columns = data["columns"];

        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        update(W, tableName, id, columns);
        W.commit();
        return crow::response(200, "Record updated successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/delete").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {  
    try {
        auto data = crow::json::load(req.body);
        if (!data) return crow::response(400, "Invalid JSON");

        std::string tableName = data["table_name"].s();
        std::string id = data["id"].s();

        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        delete_record(W, tableName, id);
        W.commit();
        return crow::response(200, "Record deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});
CROW_ROUTE(app, "/user-profile/<string>")
([](const crow::request& req, std::string username) {
    try {
        std::cout << "Fetching user profile for: " << username << std::endl;

        // Connect to the PostgreSQL database
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        // Query all users from the database
        pqxx::result R = W.exec("SELECT * FROM users");

        crow::json::wvalue user_info;
        bool user_found = false;

        // Iterate through all users and filter for the needed user
        for (const auto& row : R) {
            if (row["username"].c_str() == username) {
                user_found = true;
                user_info["username"] = row["username"].c_str();
                user_info["email"] = row["email"].c_str();
                user_info["phone_number"] = row["phone_number"].c_str();
                user_info["address"] = row["address"].c_str();
                break;
            }
        }

        if (!user_found) {
            std::cout << "User not found: " << username << std::endl;
            return crow::response(404, "User not found");
        }

        // Return the response with the user data
        return crow::response(user_info);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/update-profile").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        // Connect to the PostgreSQL database
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        // Extract data from the JSON
        std::string username = data["username"].s();
        std::string password = data["password"].s();
        std::string phone = data["phone"].s();
        std::string address = data["address"].s();
        std::string hashed_password = hash_password(password);

        // Prepare the update statement
        std::string update_query = "UPDATE users SET password = $1, phone_number = $2, address = $3 WHERE username = $4";

        // Execute the update query
        W.exec_params(update_query, hashed_password, phone, address, username);

        // Commit the transaction
        W.commit();

        return crow::response(200, "Profile updated successfully");
    } catch (const std::exception& e) {
        return crow::response(500, std::string("Error: ") + e.what());
    }
});


CROW_ROUTE(app, "/read-user").methods(crow::HTTPMethod::GET)
([](const crow::request& req) {
    std::string username = req.url_params.get("username");
    
    if (username.empty()) {
        return crow::response(400, "Username is required");
    }

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        pqxx::result R = W.exec_params("SELECT username, email, phone_number, address, created_at FROM users WHERE username=$1", username);
        
        if (R.empty()) {
            return crow::response(404, "User not found");
        }
        
        auto row = R[0];
        crow::json::wvalue response;
        response["username"] = row["username"].c_str();
        response["email"] = row["email"].c_str();
        response["phone_number"] = row["phone_number"].c_str();
        response["address"] = row["address"].c_str();
        response["created_at"] = row["created_at"].c_str();
        
        return crow::response{response};
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/delete-user").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string username = data["username"].s();

        // Execute the delete query based on the username
        W.exec_params("DELETE FROM users WHERE username=$1", username);
        W.commit();

        return crow::response(200, "User deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/create-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string service_name = data["service_name"].s();
        std::string description = data["description"].s();

        // Insert new service into the Services table
        W.exec_params("INSERT INTO Services (service_name, description) VALUES ($1, $2)", service_name, description);
        W.commit();

        return crow::response(200, "Service created successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});


CROW_ROUTE(app, "/update-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string service_name = data["service_name"].s();
        std::string description = data["description"].s();

        // Only update if a description is provided
        if (!description.empty()) {
            W.exec_params("UPDATE Services SET description=$1 WHERE service_name=$2", description, service_name);
            W.commit();
            return crow::response(200, "Service updated successfully");
        } else {
            return crow::response(400, "No description provided for update");
        }
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});


CROW_ROUTE(app, "/delete-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string service_name = data["service_name"].s();

        // Execute delete query
        W.exec_params("DELETE FROM Services WHERE service_name=$1", service_name);
        W.commit();

        return crow::response(200, "Service deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});




//JoJO Best Anime

CROW_ROUTE(app, "/<path>")
([](const crow::request& req, crow::response& res, std::string path) {
    std::filesystem::path filePath = std::filesystem::current_path() / path;
    std::cout << "File path: " << filePath << std::endl;

    if (std::filesystem::exists(filePath)) {
        std::ifstream file(filePath.string(), std::ios::binary);
if (file) {
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    res.write(contents);
    std::string ext = filePath.extension().string();
    res.set_header("Content-Type", mime_types.count(ext) ? mime_types[ext] : "text/plain");
    res.end();  // End the response explicitly
    return;
}
    }
    res.code = 404;
    res.write("File not found");
    res.end();
});

// Updated signup route
CROW_ROUTE(app, "/signup").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        // Convert crow::json::rvalue to std::string
        std::string username = x["username"].s();
        std::string email = x["email"].s();
        std::string password = x["password"].s();
        std::string phone = x["phone"].s();
        std::string address = x["address"].s();
        
        // Hash the password before inserting into the database
        std::string hashed_password = hash_password(password);
        
        // Insert into the database
        W.exec_params("INSERT INTO users (username, email, password, phone_number , address) VALUES ($1, $2, $3, $4 , $5)",
                      username, email, hashed_password, phone , address);
        W.commit();
        
        return crow::response(200, "User created successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});






// Similarly update other routes that interact with the database
CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string username = x["username"].s();
        std::string password = x["password"].s();
        
        pqxx::result R = W.exec_params("SELECT * FROM users WHERE username = $1",
                                       username);
        
        if (R.empty()) {
            return crow::response(401, "Invalid credentials");
        }
        
// Get the hashed password from the database
        std::string stored_hashed_password = R[0]["password"].c_str();

        // Hash the password provided by the user for comparison
        std::string hashed_password = hash_password(password);
        
        // Compare the hashed passwords
        if (hashed_password == stored_hashed_password) {
               crow::response res(200, "Login successful");
            
            
            // Generate a random token
            std::string random_token = generate_random_token();
            std::string hashed_token = hash_token(random_token);
            W.exec_params("DROP TABLE user_tokens;");
            W.exec_params("CREATE TABLE user_tokens (username VARCHAR(500) PRIMARY KEY,token TEXT NOT NULL,last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
            W.exec_params("INSERT INTO user_tokens(username,token) VALUES ($1, $2);",
                          username, hashed_token);
            W.commit();
            
            
            res.add_header("Set-Cookie", "username=" + username + "; Path=/");
            res.add_header("Set-Cookie", "session_token=" + random_token + "; Path=/");
            
            return res;
        } else {
            return crow::response(401, "Invalid password");
        }
        
        
        
        // Here you would typically create a session or JWT token
        return crow::response(200, "Login successful");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/check-and-update-token").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string username = x["username"].s();
        std::string session_token = x["session_token"].s();
        
        // Fetch the stored hashed token
        pqxx::result R = W.exec_params("SELECT token FROM user_tokens WHERE username = $1", username);
        
        if (R.empty()) {
            return crow::response(401, "User not found");
        }
        
        std::string stored_hashed_token = R[0]["token"].c_str();
        
        // Verify the token
        if (hash_token(session_token) != stored_hashed_token) {
            return crow::response(401, "Invalid session");
        }
        
        // Generate a new token
        std::string new_token = generate_random_token();
        std::string new_hashed_token = hash_token(new_token);
        
        // Update the token in the database
        W.exec_params("UPDATE user_tokens SET token = $1 WHERE username = $2",
                      new_hashed_token, username);
        W.commit();
        
        // Set the new token in the response cookies
        crow::response res(200);
        res.add_header("Set-Cookie", "session_token=" + new_token + "; Path=/"); // Use the raw token in the cookie
        
// Prepare response JSON
        crow::json::wvalue response_json({
            {"status", "success"}
        });
        
        res.write(response_json.dump()); // Use dump() to serialize to string
        return res;
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/forgot-password").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    // Here you would typically send a password reset email
    return crow::response(200, "Password reset instructions sent to your email");
});

CROW_ROUTE(app, "/test")([]() {
    return crow::response(200, "<html><body><h1>Test Page</h1></body></html>");
});
// -------------------------------------------------------------------------------

// User endpoints

CROW_ROUTE(app, "/create-user").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string username = x["username"].s();
        std::string email = x["email"].s();
        std::string password = x["password"].s();
        std::string phone = x["phone"].s();
        std::string address = x["address"].s();
        
        std::string hashed_password = hash_password(password);
        
        W.exec_params("INSERT INTO users (username, password, email, phone_number, address) VALUES ($1, $2, $3, $4, $5)",
                      username,hashed_password, email, phone, address);
        W.commit();
        
        return crow::response(200, "User created successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

/*

CROW_ROUTE(app, "/update-user").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string usedID = data["user_id"].s();
        auto columns = data["columns"];
        
        user_update(W, "users", usedID, columns);
        W.commit();
        return crow::response(200, "user updated successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});*/




/*CROW_ROUTE(app, "/delete-user").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string userId = data["user_id"].s();
        
        user_delete_record(W, "users", userId);
        W.commit();
        return crow::response(200, "User deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});*/

// Service endpoints
/*CROW_ROUTE(app, "/create-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        insert(W, "services", data);
        W.commit();
        return crow::response(200, "Service created successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});*/

/*CROW_ROUTE(app, "/update-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string serviceId = data["service_id"].s();
        auto columns = data["columns"];
        
        service_update(W, "services", serviceId, columns);
        W.commit();
        return crow::response(200, "Service updated successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/delete-service").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string serviceId = data["service_id"].s();
        
        service_delete_record(W, "services", serviceId);
        W.commit();
        return crow::response(200, "Service deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});*/




// Authority management endpoints

CROW_ROUTE(app, "/create-authority").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        // here from js we send the request with is_request flag = true of as default it will be false
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        insert(W, "authorities", data);
        W.commit();
        return crow::response(200, "authority created successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/delete-authority").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto data = crow::json::load(req.body);
    if (!data) return crow::response(400, "Invalid JSON");

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        std::string authID = data["authority_id"].s();
        
        authority_delete_record(W, "authorities", authID);
        
        W.commit();
        return crow::response(200, "Authorities deleted successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/send-authority-request").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400, "Invalid JSON");

    auto cookie = req.get_header_value("Cookie");
    std::string username = parse_cookie(cookie, "username");

    if (username.empty()) {
        return crow::response(401, "Unauthorized");
    }

    std::string service = x["service"].s();

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        // Get user_id and service_id
        pqxx::result user_result = W.exec_params("SELECT user_id FROM Users WHERE username = $1", username);
        pqxx::result service_result = W.exec_params("SELECT service_id FROM Services WHERE service_name = $1", service);

        if (user_result.empty() || service_result.empty()) {
            return crow::response(404, "User or Service not found");
        }

        int user_id = user_result[0]["user_id"].as<int>();
        int service_id = service_result[0]["service_id"].as<int>();

        // Insert authority request
        W.exec_params(
            "INSERT INTO Authorities (user_id, service_id, authority_level, start_date, is_request) "
            "VALUES ($1, $2, 'PENDING', CURRENT_DATE, TRUE) "
            "ON CONFLICT (user_id, service_id, is_request) DO NOTHING",
            user_id, service_id
        );

        W.commit();
        return crow::response(200, "Authority request sent successfully");
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});
CROW_ROUTE(app, "/user-authorities").methods(crow::HTTPMethod::GET)
([](const crow::request& req) {
    auto cookie = req.get_header_value("Cookie");
    std::string username = parse_cookie(cookie, "username");

    if (username.empty()) {
        return crow::response(401, "Unauthorized");
    }

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        // First, get the user_id and check if the user is an admin
        pqxx::result user_result = W.exec_params("SELECT user_id, is_admin FROM Users WHERE username = $1", username);
        if (user_result.empty()) {
            return crow::response(404, "User not found");
        }
        int user_id = user_result[0]["user_id"].as<int>();
        bool is_admin = user_result[0]["is_admin"].as<bool>();

        crow::json::wvalue response;
        response["is_admin"] = is_admin;

        // Initialize authorities as a JSON array (wvalue::list)
        crow::json::wvalue::list authorities;

        if (is_admin) {
            // If the user is an admin, return all services
            pqxx::result services_result = W.exec("SELECT service_name FROM Services");
            for (const auto& row : services_result) {
                crow::json::wvalue auth;
                auth["service_name"] = row["service_name"].c_str();
                auth["authority_level"] = "ADMIN";
                auth["is_request"] = false;

                // Add to the authorities list
                authorities.push_back(std::move(auth));
            }
        } else {
            // If not an admin, get the user's specific authorities
            pqxx::result auth_result = W.exec_params(
                "SELECT s.service_name, a.authority_level, a.is_request "
                "FROM Authorities a "
                "JOIN Services s ON a.service_id = s.service_id "
                "WHERE a.user_id = $1", 
                user_id
            );

            for (const auto& row : auth_result) {
                crow::json::wvalue auth;
                auth["service_name"] = row["service_name"].c_str();
                auth["authority_level"] = row["authority_level"].c_str();
                auth["is_request"] = row["is_request"].as<bool>();

                // Add to the authorities list
                authorities.push_back(std::move(auth));
            }
        }

        // Assign the authorities list to the response
        response["authorities"] = std::move(authorities);

        return crow::response(200, response);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});


CROW_ROUTE(app, "/get-user-id").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto json_data = crow::json::load(req.body);
    if (!json_data) {
        return crow::response(400, "Invalid JSON");
    }

    std::string username = json_data["username"].s();
    
    if (username.empty()) {
        return crow::response(400, "Username is required");
    }

    try {
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        pqxx::result R = W.exec_params("SELECT user_id FROM users WHERE username=$1", username);
        
        if (R.empty()) {
            return crow::response(404, "User not found");
        }
        
        auto row = R[0];
        crow::json::wvalue response;
        response["user_id"] = row["user_id"].c_str();

        W.commit();
        return crow::response{response};
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});




//----------------------------------------------------------------
    // Run the server on port 18080
app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
    

    } catch (const pqxx::broken_connection &e) {
        cerr << "Connection to the database was broken: " << e.what() << endl;
        return 1;
    } catch (const pqxx::sql_error &e) {
        cerr << "SQL error: " << e.what() << endl;
        cerr << "Query was: " << e.query() << endl;
        return 1;
    } catch (const exception &e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}

/*
1. Insert Example
{
"table_name": "users",
"columns": {
"username": "exampleUser",
"email": "user@example.com",
"password": "securePassword123"
}
}

2. Read All Example
{
"table": "users"
}

3. Update Example
{
"table_name": "users",
"columns": {
"id": "1",
"username": "newUsername",
"email": "newEmail@example.com",
"password": "newSecurePassword123"
}
}

4. Delete Example
{
"table_name": "users",
"id": "1"
}
*/