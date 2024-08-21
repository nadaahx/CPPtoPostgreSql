#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <sstream>
#include "crow_all.h"
using namespace std;

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
void create_table(pqxx::work &W, const string &table_name, Columns... columns) {
    // Create a vector of column definitions as strings
    vector<string> column_definitions;
    (..., column_definitions.push_back(columns));  // Fold expression to collect column definitions

    // Join all column definitions into a single string
    string column_definitions_str = join(column_definitions, ", ");

    // Construct the SQL query
    string query = "CREATE TABLE IF NOT EXISTS " + table_name + " (id SERIAL PRIMARY KEY, " + column_definitions_str + ");";
    
    // Execute the query
    W.exec0(query);
    cout << "Table '" << table_name << "' created or already exists with columns: " << column_definitions_str << endl;
}

// Function to extract column and value from "column value" format
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
    vector<string> columns;
    vector<string> values;

    // Process each argument and separate into columns and values
    (void)std::initializer_list<int>{( [&] {
        auto [column, value] = parse_column_value(args);
        columns.push_back(column);
        values.push_back(W.quote(value)); // Use pqxx's quote method to handle quoting
    }(), 0)... };

    // Construct the SQL query
    string columns_str = join(columns, ", ");
    string values_str = join(values, ", ");
    string query = "INSERT INTO " + table + " (" + columns_str + ") VALUES (" + values_str + ");";

    // Execute the query
    W.exec0(query);
    cout << "Inserted into " << table << " (" << columns_str << "): " << values_str << endl;
}



template<typename T>
void update(pqxx::work &W, const string &table, const string &column, int id, const T &new_value) {
    W.exec0("UPDATE " + table + " SET " + column + " = " + W.quote(new_value) + " WHERE id = " + W.quote(to_string(id)) + ";");
    
    cout << "Updated in " << table << ": ID " << id << " set " << column << " to " << new_value << endl;
}

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
        pqxx::connection C("dbname=mydb user=jojo password=jojo hostaddr=127.0.0.1 port=5432");

        if (C.is_open()) {
            cout << "Connected to " << C.dbname() << endl;
        } else {
            cerr << "Failed to connect to database" << endl;
            return 1;
        }

        pqxx::work W(C);

        create_table(W, "anime_girls", "name VARCHAR(50)", "rating FLOAT","anime VARCHAR(50)");


        insert(W, "anime_girls", "name 'Nami'", "rating 30.1", "anime 'OP'");
        read_all_rows(W, "anime_girls"); // Print all rows after insert

        update<string>(W, "anime_girls", "name", 1, "Robin");
        read_all_rows(W, "anime_girls"); // Print all rows after update

        delete_record<string> (W, "anime_girls", 1);
        read_all_rows(W, "anime_girls"); // Print all rows after delete


        W.commit();
        

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







    //-------- FOR NADA


    // Create an instance of Crow application
    crow::SimpleApp app;

    // Define a route for the root URL
    app.route_dynamic("/")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, crow::response& res) {
        res.write("Hello, World!");
        res.end();
    });

    // Run the server on port 18080
    app.port(18080).multithreaded().run();

    return 0;
}
