#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>
#include "crow_all.h"
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

// Function to split a string by a delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Function to extract the number from a column name as a string
std::string extract_number(const std::string& column_name) {
    std::string number_str;
    // Traverse the string from the end to find the number
    for (auto it = column_name.rbegin(); it != column_name.rend(); ++it) {
        if (std::isdigit(*it)) {
            number_str = *it + number_str;  // prepend digit
        } else if (!number_str.empty()) {
            break;  // stop when we have collected the number
        }
    }
    return number_str.empty() ? "" : number_str;
}

// Function to extract the number from columns
std::string get_id_from_columns(const std::string& json_data) {
    // Locate the start and end of the columns array
    size_t columns_start = json_data.find("\"columns\": [") + 12;  // 12 is the length of "\"columns\": ["
    if (columns_start == std::string::npos) return "";

    size_t columns_end = json_data.find("]", columns_start);
    if (columns_end == std::string::npos) return "";

    // Extract the columns substring
    std::string columns_str = json_data.substr(columns_start, columns_end - columns_start);

    // Remove leading/trailing whitespace and quotes
    columns_str.erase(remove(columns_str.begin(), columns_str.end(), '\"'), columns_str.end());

    // Split the columns string by commas
    auto columns = split(columns_str, ',');

    // Find and return the number from the first column containing "id"
    for (auto& column : columns) {
        // Trim leading/trailing whitespace from each column
        column.erase(column.find_last_not_of(" \n\r\t") + 1);
        column.erase(0, column.find_first_not_of(" \n\r\t"));

        if (column.find("id") != std::string::npos) {
            return extract_number(column);
        }
    }

    return "";  // Return empty string if no number is found
}

void parseTableSchema(const std::string& schema, std::string& tableName, std::vector<std::string>& columns) {
    std::string::size_type tableNameStart = schema.find("table_name") + 11; // Length of "table_name" + 2 for ": "
    std::string::size_type tableNameEnd = schema.find(","); // Find the end of table_name
    tableName = schema.substr(tableNameStart, tableNameEnd - tableNameStart);
    
    // Remove quotes from tableName
    if (!tableName.empty() && tableName.front() == '"' && tableName.back() == '"') {
        tableName = tableName.substr(1, tableName.length() - 2);
    }

    std::string::size_type columnsStart = schema.find("columns") + 10; // Length of "columns" + 2 for ": "
    std::string::size_type columnsEnd = schema.find("]", columnsStart) + 1;
    std::string columnsPart = schema.substr(columnsStart, columnsEnd - columnsStart);

    // Remove brackets and split columns
    columnsPart.erase(0, 1); // Remove leading '['
    columnsPart.erase(columnsPart.size() - 1, 1); // Remove trailing ']'

    std::istringstream stream(columnsPart);
    std::string column;

    while (std::getline(stream, column, ',')) {
        // Trim leading and trailing spaces
        column.erase(0, column.find_first_not_of(' '));
        column.erase(column.find_last_not_of(' ') + 1);

        // Remove quotes if present
        if (!column.empty() && column.front() == '"' && column.back() == '"') {
            column = column.substr(1, column.length() - 2);
        }

        columns.push_back(column);
    }
}

// Helper function to concatenate strings with a delimiter
string join(const vector<string>& elements, const string& delimiter) {
    stringstream ss;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) ss << delimiter;
        ss << elements[i];
    }
    return ss.str();
}

// Variadic template function to create a table with specified columns
template<typename... Columns>
void create_table(pqxx::work& work, const std::string& table_name, Columns&&... columns) {
    // Collect column definitions into a vector
    std::vector<std::string> column_definitions = { std::forward<Columns>(columns)... };

    // Join all column definitions into a single string
    std::string column_definitions_str = join(column_definitions, ", ");

    // Construct the SQL query
    std::string query = "CREATE TABLE IF NOT EXIST " + table_name + " (id SERIAL PRIMARY KEY, " + column_definitions_str + ")";
    query.erase(std::remove(query.begin(), query.end(), ':'), query.end());
    cout<<query<<endl;
    // Execute the query
    work.exec0(query);
    std::cout << "Table '" << table_name << "' created or already exists with columns: " << column_definitions_str << std::endl;
}


// // Function to extract column and value from "column value" format
tuple<string, string> parse_column_value(const string& column_value) {
    size_t pos = column_value.find(' ');
    if (pos == string::npos) {
        throw invalid_argument("Invalid format. Expected 'column value'.");
    }
    string column = column_value.substr(0, pos);
    string value = column_value.substr(pos + 1);
    return make_tuple(column, value);
}
// Edited the name to insert


// Variadic template function to insert values into a table
template<typename... Args>
void insert(pqxx::work &W, const string &table, Args... args) {
    std::vector<std::string> args_def = { std::forward<Args>(args)... };

    vector<string> columns;
    vector<string> values;

    for(auto x : args_def){
        auto [column , value ] = parse_column_value(x);
        columns.push_back(column);
        values.push_back(W.quote(value)); 

    }
    string columns_str = join(columns, ", ");
    string values_str = join(values, ", ");
    cout<<columns_str<<endl;
    cout<<values_str<<endl;

    string query = "INSERT INTO " + table + " (" + columns_str + ") VALUES (" + values_str + ");";
    cout<<query<<endl;
    query.erase(std::remove(query.begin(), query.end(), ':'), query.end());
    // Execute the query
    W.exec0(query);
    cout << "Inserted into " << table << " (" << columns_str << "): " << values_str << endl;
}


template<typename... T>
void update(pqxx::work &W, const std::string &table, string id, T&&... ts) {
    std::vector<std::string> args_def = { std::forward<T>(ts)... };

    vector<string> columns;
    vector<string> values;
    for(auto x : args_def){
        auto [column , value ] = parse_column_value(x);
        columns.push_back(column);
        values.push_back(W.quote(value)); 

    }
    string str="";
    for( int i=0 ; i < columns.size() ; i++){
        if(columns[i]=="id"){
            continue;
        }
        str+=columns[i];
        str+=" = ";
        str+=values[i];
        if(i!=columns.size()-1){
            str+=", ";
        }
        
    }

    std::string query = "UPDATE " + table + " SET " + str + " WHERE id = " + id+ ";";
    query.erase(std::remove(query.begin(), query.end(), ':'), query.end());
    cout<<query<<endl;

    W.exec0(query);
}


void delete_record(pqxx::work &W, const string &table, string id) {
    string query ="DELETE FROM " + table + " WHERE id = " + id+ ";";
    query.erase(std::remove(query.begin(), query.end(), ':'), query.end());

    W.exec0(query);
   
    cout << "Deleted from " << table << ": ID " << id << endl;
}

// Edited the name instead of read
void read_all_rows(pqxx::work &W, const string &table) {
    pqxx::result R = W.exec("SELECT * FROM " + table + ";");
    cout << "Rows in table '" << table << "':" << endl;
    for (const auto &row : R) {
        for (const auto &field : row) {
            cout << field.c_str() << " ";
        }
        cout << endl;
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
}

int main() {
    try {

        //curl -X POST http://localhost:18080/ -H "Content-Type: application/json" -d '{"username":"user", "password":"password"}'


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

    app.route_dynamic("/create_table")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, crow::response& res) {
        protectedRoute(req);
        std::string data = req.body;

    std::string tableName;
    std::vector<std::string> columns;
    // data should be as follow: table_name: "table_name_value", columns: [ "column1 dataType", "column2 dataType", ..., "columnN dataType" ]
    parseTableSchema(data, tableName, columns);

    pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
    pqxx::work W(C);
    create_table(W, tableName, columns);
    W.commit();
    res.write("Table created successfully");
    res.end();
    });


    app.route_dynamic("/insert")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, crow::response& res) {
        protectedRoute(req);
        std::string data = req.body;

    std::string tableName;
    std::vector<std::string> columns;
    // data should be as follow: table_name: "table_name_value", columns: ["column_name value", "column_name value",....., "column_name value"]}'
    parseTableSchema(data, tableName, columns);
    pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
    pqxx::work W(C);
    insert(W, tableName, columns);
    W.commit();
    res.write("Inserted Into Table created successfully");
    res.end();
    });

app.route_dynamic("/read_all")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, crow::response& res) {
        protectedRoute(req);
        std::string tableName = req.url_params.get("table");
        if (tableName.empty()) {
            res.write("Table name is required");
            res.end();
            return;
        }

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        std::ostringstream oss;
        pqxx::result R = W.exec("SELECT * FROM " + tableName + ";");
        for (const auto& row : R) {
            for (const auto& field : row) {
                oss << field.c_str() << " ";
            }
            oss << "\n";
        }
        res.write(oss.str());
        res.end();
    });

app.route_dynamic("/update")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, crow::response& res) {
        protectedRoute(req);
    std::string data = req.body;
    string id = get_id_from_columns(data);
    std::string tableName;
    std::vector<std::string> columns;
    // data should be as follow: table_name: "table_name_value", columns: ["column_name value", "column_name value",....., "column_name value"]}'
    // be carefull that determenant of the (where) is column called id (static name)
    parseTableSchema(data, tableName, columns);

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        update(W, tableName , id, columns);
        W.commit();
        res.write("Record updated successfully");
        res.end();
    });

      app.route_dynamic("/delete")
        .methods(crow::HTTPMethod::POST)
        ([](const crow::request& req, crow::response& res) {
            protectedRoute(req);
            std::string data = req.body;
    string id = get_id_from_columns(data);
    std::string tableName;
    std::vector<std::string> columns;
    // data should be as follow: table_name: "table_name_value", columns: ["id value"]}'
    // be carefull that determenant of the (where) is column called id (static name)
    parseTableSchema(data, tableName, columns);

            pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
            pqxx::work W(C);
            delete_record(W, tableName, id);
            W.commit();
            res.write("Record deleted successfully");
            res.end();
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
