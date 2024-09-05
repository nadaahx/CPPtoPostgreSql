/*

this is a simple API that insert into postgreSQl table that is exist already but with authentication using JWT
run with: 
    g++ main_it.cpp -o output -I/root/Nada/Database_connection/include -lpqxx -lpq  -lssl -lcrypto
then : 
    ./output
    
test using:

http://38.242.215.0:18080/login?username=user1&password=password123

http://38.242.215.0:18080/insert/users?username=john_doe&email=john.doe@example.com&password=supersecurepassword
*/



#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include "crow_all.h"
#include "jwt_auth.h"
#include <stdexcept>

using namespace std;


const std::string TOKEN_FILE_PATH = "/root/Nada/Database_connection/testing/cookies_jwt/token.txt";  // Specify your token file path

void clear_cookies(crow::response& res) {
    // Set cookie with an expired date to clear it
    res.add_header("Set-Cookie", "jwt=; expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly; Path=/");

    // Clear the token from the file
    std::ofstream tokenFile(TOKEN_FILE_PATH, std::ios::trunc); // Open file in truncate mode to clear its content
    if (!tokenFile) {
        cerr << "Error opening token file for clearing." << endl;
    } else {
        tokenFile.close(); // Close the file after truncating
    }
}


void insert(pqxx::work& W, const std::string& tableName, const crow::query_string& params) {
    std::string query = "INSERT INTO " + tableName + " (";
    std::string values = "VALUES (";
    
    bool first = true;
    for (const auto& key : params.keys()) {
        if (key != "table_name") {
            if (!first) {
                query += ", ";
                values += ", ";
            }
            query += key;
            values += W.quote(params.get(key));
            first = false;
        }
    }
    
    query += ") " + values + ")";
    
    if (first) {
        throw std::runtime_error("No valid columns provided for insertion");
    }
    
    W.exec(query);
}

int main() {
    crow::SimpleApp app;
    JWTAuth jwt_auth;
    
     crow::response clearRes;
    clear_cookies(clearRes);
    
    CROW_ROUTE(app, "/")
    ([](){
        return "Hello, World!";
    });

    CROW_ROUTE(app, "/login").methods("GET"_method, "POST"_method)
    ([&jwt_auth](const crow::request& req) {
        if (req.method == crow::HTTPMethod::GET) {
            // For GET requests, create a new request object with JSON body
            crow::request modified_req;
            modified_req.method = crow::HTTPMethod::POST;
            crow::json::wvalue json_body;
            json_body["username"] = req.url_params.get("username") ? req.url_params.get("username") : "";
            json_body["password"] = req.url_params.get("password") ? req.url_params.get("password") : "";
            modified_req.body = json_body.dump();
            return jwt_auth.login(modified_req);
        } else {
            // For POST requests, use the original request
            return jwt_auth.login(req);
        }
    });

CROW_ROUTE(app, "/insert/<string>")
([&jwt_auth](const crow::request& req, std::string table_name) {
    // Extract JWT token from cookies
    std::string token = req.get_header_value("Cookie");
    size_t pos = token.find("jwt=");
    if (pos != std::string::npos) {
        pos += 4; // Skip "jwt=" part
        size_t end_pos = token.find(";", pos);
        if (end_pos != std::string::npos) {
            token = token.substr(pos, end_pos - pos);
        } else {
            token = token.substr(pos);
        }
    } else {
        return crow::response(401, "Unauthorized: No token found in cookies");
    }

    // Modify the request to include the Authorization header with the token
    crow::request modified_req = req;
    modified_req.add_header("Authorization", "Bearer " + token);

    // Validate the token using the JWTAuth class
    if (!jwt_auth.protect_route(modified_req)) {
        return crow::response(401, "Unauthorized: Invalid token");
    }

    try {
        if (table_name.empty()) {
            return crow::response(400, "Missing table name");
        }
        
        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        insert(W, table_name, req.url_params);
        W.commit();
        
        return crow::response(200, "Inserted into table successfully");
    } catch (const std::exception& e) {
        return crow::response(500, std::string("Error: ") + e.what());
    }
});


    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
    return 0;
}