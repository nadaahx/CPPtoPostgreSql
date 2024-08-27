#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>
#include "crow_all.h"
#include <stdexcept>
using namespace std;



string generate_secure_key(int length) {
    vector<unsigned char> buffer(length);
    if (RAND_bytes(buffer.data(), length) != 1) {
        throw runtime_error("Failed to generate secure key");
    }
    return string(buffer.begin(), buffer.end());
}

const string secure_key = generate_secure_key(32);

std::string create_jwt(const std::string& username) {
    return jwt::create()
        .set_issuer("auth_server")
        .set_subject(username)
        .set_payload_claim("username", jwt::claim(username))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{ 3600 })
        .sign(jwt::algorithm::hs256{ secure_key });
}

// Function to validate JWT
bool validateJWT(const std::string& token) {
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

auto protectedRoute(const crow::request& req) {
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.substr(0, 7) == "Bearer ") {
        std::string token = authHeader.substr(7);
        if (validateJWT(token)) {
            return true; // Token is valid
        } else {


            throw crow::response(401, "Invalid token");
        }
    } else {
        throw crow::response(401, "Missing token");
    }
    //cout<<"helloworld";
}


void create_table(pqxx::work& W, const std::string& tableName, const std::vector<std::string>& columns) {
    std::string query = "CREATE TABLE IF NOT EXISTS " + tableName + " ("
                        "id SERIAL PRIMARY KEY, "
                        "created_at DATE DEFAULT CURRENT_DATE, "; // Adding the date column
                        // yyyy/mm/dd

    for (const auto& column : columns) {
        query += column + ", ";
    }
    query = query.substr(0, query.length() - 2) + ");";
    W.exec(query);
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



void delete_record(pqxx::work& W, const std::string& tableName, const std::string& id) {
    std::string query = "DELETE FROM " + tableName + " WHERE id = " + id + ";";
    W.exec(query);
}

std::vector<pqxx::row> get_data_by_date_range(pqxx::work& W, const std::string& tableName, const std::string& startDate, const std::string& endDate) {
    std::string query = "SELECT * FROM " + tableName + " WHERE created_at BETWEEN " + 
                        W.quote(startDate) + " AND " + W.quote(endDate) + ";";
    pqxx::result r = W.exec(query);
    std::vector<pqxx::row> data;

    for (const auto& row : r) {
        data.push_back(row);
    }
    
    return data;
}

std::vector<pqxx::row> get_data_by_id_range(pqxx::work& W, const std::string& tableName, int startId, int endId) {
    std::string query = "SELECT * FROM " + tableName + " WHERE id BETWEEN " + 
                        std::to_string(startId) + " AND " + std::to_string(endId) + ";";
    pqxx::result r = W.exec(query);
    std::vector<pqxx::row> data;

    for (const auto& row : r) {
        data.push_back(row);
    }
    
    return data;
}



int main() {
    try {


    crow::SimpleApp app;



app.route_dynamic("/")
.methods(crow::HTTPMethod::POST)
([](const crow::request& req){
    auto x = crow::json::load(req.body);
    if (!x) return crow::response(400);

    std::string username = x["username"].s();
    std::string password = x["password"].s();
    CROW_LOG_INFO << "user received: " << x;
    CROW_LOG_INFO << "pass received: " << password;

    if (username == "test" && password == "testpass") {
        std::string token = create_jwt(username);
        return crow::response(200 , token);
    } else {
        return crow::response(401, "Invalid credentials");
    }
});



            CROW_ROUTE(app, "/create_table").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            try {
                protectedRoute(req);
                auto data = crow::json::load(req.body);
                if (!data) return crow::response(400, "Invalid JSON");

                std::string tableName = data["table_name"].s();
                std::vector<std::string> columns;
                for (const auto& column : data["columns"]) {
                    columns.push_back(column.s());
                }

                pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
                pqxx::work W(C);
                create_table(W, tableName, columns);
                W.commit();
                return crow::response(200, "Table created successfully");
            } catch (const std::exception& e) {
                return crow::response(500, e.what());
            }
        });



     CROW_ROUTE(app, "/insert").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            try {
                protectedRoute(req);
                auto data = crow::json::load(req.body);
                if (!data) return crow::response(400, "Invalid JSON");

                std::string tableName = data["table_name"].s();
                auto columns = data["columns"];

                pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
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
                protectedRoute(req);
                auto tableName = req.url_params.get("table");
                if (!tableName) return crow::response(400, "Table name is required");

                pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
                pqxx::work W(C);
                pqxx::result R = W.exec("SELECT * FROM " + std::string(tableName) + ";");
                
                crow::json::wvalue result;
                for (size_t i = 0; i < R.size(); ++i) {
                    crow::json::wvalue row;
                    for (size_t j = 0; j < R[i].size(); ++j) {
                        row[R[i][j].name()] = R[i][j].c_str();
                    }
                    result[i] = std::move(row);
                }
                return crow::response(200, result);
            } catch (const std::exception& e) {
                return crow::response(500, e.what());
            }
        });


  CROW_ROUTE(app, "/update").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            try {
                protectedRoute(req);
                auto data = crow::json::load(req.body);
                if (!data) return crow::response(400, "Invalid JSON");

                std::string tableName = data["table_name"].s();
                std::string id = data["columns"]["id"].s();
                auto columns = data["columns"];

                pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
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
                protectedRoute(req);
                auto data = crow::json::load(req.body);
                if (!data) return crow::response(400, "Invalid JSON");

                std::string tableName = data["table_name"].s();
                std::string id = data["id"].s();

                pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
                pqxx::work W(C);
                delete_record(W, tableName, id);
                W.commit();
                return crow::response(200, "Record deleted successfully");
            } catch (const std::exception& e) {
                return crow::response(500, e.what());
            }
        });
    // Run the server on port 18080
    app.port(18080).multithreaded().run();

    

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

