/*

this is a simple API that insert into postgreSQl table that is exist already but with authentication using JWT and simple frontend
run with: 
 g++ main_it.cpp -o output -I/root/Nada/project/version_2/include -lpqxx -lpq  -lssl -lcrypto

then : 
    ./output
    
test using:

http://38.242.215.0:18080/login
http://38.242.215.0:18080/insert_profile
*/



#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include "crow_all.h"
#include "jwt_auth.h"
#include <stdexcept>

using namespace std;



std::string readHTMLFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;

    if (file) {
        buffer << file.rdbuf(); // Read the entire file into the buffer
        return buffer.str();    // Return the file contents as a string
    } else {
        return "Error loading file!";
    }
}


const std::string TOKEN_FILE_PATH = "/root/Nada/project/version_2/cookies_frontend/user1_token.txt";  // Specify your token file path

void clear_cookies(crow::response& res) {
    // Set cookie with an expired date to clear it
    // removed the HttpOnly that caused the CORS error
    res.add_header("Set-Cookie", "jwt=; expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/");

    // Clear the token from the file/
    std::ofstream tokenFile(TOKEN_FILE_PATH, std::ios::trunc); // Open file in truncate mode to clear its content
    if (!tokenFile) {
        cerr << "Error opening token file for clearing." << endl;
    } else {
        tokenFile.close(); // Close the file after truncating
    }
}

void insert(pqxx::work& W, const std::string& table_name, const crow::json::rvalue& json_body) {
    // Extracting data from json_body
    std::string user_id = json_body["user_id"].s();
    std::string address = json_body["address"].s();

    // Prepare your SQL statement
    std::string sql = "INSERT INTO " + table_name + " (user_id, address) VALUES (" +
                      W.quote(user_id) + ", " + W.quote(address) + ");";

    // Execute the SQL statement
    W.exec(sql);
}


int main() {
    crow::SimpleApp app;
    JWTAuth jwt_auth;
    
     crow::response clearRes;
     clear_cookies(clearRes);
    
    
    
    CROW_ROUTE(app, "/")
([](const crow::request& req, crow::response& res){
    res.add_header("Access-Control-Allow-Origin", "*"); // Or use specific origin
    res.add_header("Access-Control-Allow-Headers", "Content-Type");
    res.write("Hello, World!");
    res.end();
});


CROW_ROUTE(app, "/login").methods("GET"_method, "POST"_method)
([&jwt_auth](const crow::request& req) {
    crow::response res;
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");

    if (req.method == crow::HTTPMethod::OPTIONS) {
        return res;
    }

    if (req.method == crow::HTTPMethod::GET) {
        // Serve HTML login form
        res.set_header("Content-Type", "text/html");
        res.write(readHTMLFile("login.html"));
        return res;
    } else {
        // Handle POST request (existing login logic)
        return jwt_auth.login(req);
    }
});

CROW_ROUTE(app, "/check_token")
.methods("POST"_method)
([&jwt_auth](const crow::request& req) {
    crow::json::rvalue x;
    try {
        x = crow::json::load(req.body);
    } catch (const std::runtime_error& e) {
        CROW_LOG_ERROR << "JSON parsing error: " << e.what();
        return crow::response(400, "Invalid JSON format");
    }

    if (!x) {
        CROW_LOG_ERROR << "Invalid JSON: " << req.body;
        return crow::response(400, "Invalid JSON");
    }

    std::string username, token;
    CROW_LOG_ERROR << "Received JSON: " << req.body;
try {
    if (!x.has("username") || !x.has("token")) { 
        CROW_LOG_ERROR << "Missing required fields in JSON";
        return crow::response(400, "Missing required fields: username and/or token");
    }
    username = x["username"].s();
    token = x["token"].s(); 
} catch (const std::runtime_error& e) {
    CROW_LOG_ERROR << "Error parsing JSON fields: " << e.what();
    return crow::response(400, "Error parsing JSON fields: " + std::string(e.what()));
}

    CROW_LOG_INFO << "Checking token for user: " << username;

    if (jwt_auth.check_and_refresh_token(username, token)) {
        std::string new_token = jwt_auth.get_new_token(username);
        crow::json::wvalue response_json({
            {"status", "success"},
            {"new_token", new_token}
        });
        return crow::response(200, response_json);
    } else {
        CROW_LOG_WARNING << "Token mismatch for user: " << username;
        return crow::response(401, "Token mismatch");
    }
});




CROW_ROUTE(app,  "/insert_profile").methods("GET"_method, "POST"_method)
([&jwt_auth](const crow::request& req) {
    crow::response res;
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Methods", "GET, POST");
    res.add_header("Access-Control-Allow-Headers", "Content-Type");

    if (req.method == crow::HTTPMethod::OPTIONS) {
        return res;  // Return response for OPTIONS method
    }

    if (req.method == crow::HTTPMethod::GET) {
        // Serve HTML login form
        res.set_header("Content-Type", "text/html");
        res.write(readHTMLFile("profile.html"));
        return res;  // Ensure response is returned for GET request
    }

    if (req.method == crow::HTTPMethod::POST) {
        auto json_body = crow::json::load(req.body);


        pqxx::connection C("dbname=mydatabase user=username password=password hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        
        std::string table_name = "profile";
        std::cout<<json_body<<std::endl;
        insert(W, table_name, json_body);
        cout<<req.url_params<<endl;
        W.commit();
        
        return crow::response(200, "Inserted into table successfully");
    }

    // Optional: Handle unexpected methods
    res.code = 405; // Method Not Allowed
    res.write("Method Not Allowed");
    return res;  // Ensure a response is returned for unsupported methods
});




    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
    return 0;
}