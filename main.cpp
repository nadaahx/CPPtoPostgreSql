#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include "crow_all.h"
using namespace std;


// bool extract_id_value(const std::vector<std::string>& columns, int& id) {
//     for (const auto& str : columns) {
//         if (str.find("id ") == 0) {  // Check if the string starts with "id "
//             std::istringstream iss(str.substr(3)); // Extract the part after "id "
//             if (iss >> id) {  // Try to extract an integer value
//                 return true;  // Successful extraction
//             }
//         }
//     }
//     return false;  // No match found or extraction failed
// }
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
    std::string query = "CREATE TABLE " + table_name + " (" + column_definitions_str + ")";
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






// template<typename T>
// void update(pqxx::work &W, const string &table, const string &column, int id, const T &new_value) {
//     W.exec0("UPDATE " + table + " SET " + column + " = " + W.quote(new_value) + " WHERE id = " + W.quote(to_string(id)) + ";");
    
//     cout << "Updated in " << table << ": ID " << id << " set " << column << " to " << new_value << endl;
// }

// template<typename... T>
// void update(pqxx::work &W, const std::string &table,int id,  T&&... ts) {
//     std::vector<std::string> args_def = { std::forward<T>(ts)... };

//     vector<string> columns;
//     vector<string> values;
//     for(auto x : args_def){
//         auto [column , value ] = parse_column_value(x);
//         columns.push_back(column);
//         values.push_back(W.quote(value)); 

//     }
//     string str="";
//     for( int i=0 ; i < columns.size() ; i++){
//         str+=columns[i];
//         str+=" = ";
//         str+=values[i];
//     }
    

//     std::string query = "UPDATE " + table + " SET " + str + " WHERE id = " + W.quote(to_string(id)) + ";";
//     W.exec0(query);
// }

template<typename T>
void delete_record(pqxx::work &W, const string &table, int id) {
    W.exec0("DELETE FROM " + table + " WHERE id = " + W.quote(to_string(id)) + ";");
   
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

int main() {
    try {
        // pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");

        // if (C.is_open()) {
        //     cout << "Connected to " << C.dbname() << endl;
        // } else {
        //     cerr << "Failed to connect to database" << endl;
        //     return 1;



        // }

        // // Run database operations
        // {
        //     // pqxx::work W(C);

        //     // read_all_rows(W, "anime_girls"); // Print all rows after insert

        //     // update<string>(W, "anime_girls", "name", 1, "Robin");
        //     // read_all_rows(W, "anime_girls"); // Print all rows after update

        //     // delete_record<string>(W, "anime_girls", 1);
        //     // W.commit();
        // }

    crow::SimpleApp app;

    // Define a route for the root URL
    app.route_dynamic("/")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, crow::response& res) {
        res.write("Hello, World!");
        res.end();
    });

    app.route_dynamic("/create_table")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, crow::response& res) {
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

    // app.route_dynamic("/delete")
    // .methods(crow::HTTPMethod::POST)
    // ([](const crow::request& req, crow::response& res) {
    //     std::string data = req.body;

    //     std::string tableName;
    //     int id;
    //     // data should be as follow: table_name: "table_name_value", id: 123
    //     std::istringstream ss(data);
    //     std::string token;
    //     while (std::getline(ss, token, ',')) {
    //         if (token.find("table_name") != std::string::npos) {
    //             size_t start = token.find(':') + 2; // Skip ": "
    //             size_t end = token.find_last_not_of('"') + 1;
    //             tableName = token.substr(start, end - start);
    //         } else if (token.find("id") != std::string::npos) {
    //             size_t start = token.find(':') + 2;
    //             id = std::stoi(token.substr(start));
    //         }
    //     }

    //     pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
    //     pqxx::work W(C);
    //     delete_record(W, tableName, id);
    //     W.commit();
    //     res.write("Record deleted successfully");
    //     res.end();
    // });

app.route_dynamic("/read_all")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, crow::response& res) {
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

// app.route_dynamic("/update")
//     .methods(crow::HTTPMethod::POST)
//     ([](const crow::request& req, crow::response& res) {
//     std::string data = req.body;

//     std::string tableName;
//     std::vector<std::string> columns;
//     int id = 0;
//     extract_id_value(columns , id);
//     // data should be as follow: table_name: "table_name_value", columns: ["column_name value", "column_name value",....., "column_name value"]}'
//     parseTableSchema(data, tableName, columns);

//         pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
//         pqxx::work W(C);
//         update(W, tableName, id , columns);
//         W.commit();
//         res.write("Record updated successfully");
//         res.end();
//     });





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
