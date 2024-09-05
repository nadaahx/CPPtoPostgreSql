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

/*
this is a simple API that insert into postgreSQl table that is exist already
run with: g++ main_i.cpp -o output -I/home/ubuntu/project/code/include -lpqxx -lpq  -lssl -lcrypto
then : ./output


test using:
curl -X POST http://0.0.0.0:18080/insert \
-H "Content-Type: application/json" \
-d '{
    "table_name": "users",
    "columns": {
        "username": "john_doe",
        "email": "john.doe@example.com",
        "password": "supersecurepassword"
    }
}'


*/




string json_value_to_string(const crow::json::rvalue& value) {
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

int main(){

    crow::SimpleApp app;

     CROW_ROUTE(app, "/insert").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
            try {
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

        app.port(18080).multithreaded().run();

        return 0;
}