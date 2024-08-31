#include <iostream>
#include <pqxx/pqxx>
#include <tuple>
#include <string>
#include <vector>
#include <sstream>
#include <openssl/rand.h>
#include <jwt-cpp/jwt.h>
#include "crow_all.h"
#include "jwt_auth.h"
#include <stdexcept>
#include <hpdf.h>
#include <xlsxwriter.h>

using namespace std;


void generate_excel(const std::vector<pqxx::row>& data, const std::string& filename) {
    lxw_workbook* workbook = workbook_new(filename.c_str());
    if (!workbook) {
        throw std::runtime_error("Failed to create Excel workbook");
    }

    lxw_worksheet* worksheet = workbook_add_worksheet(workbook, NULL);
    if (!worksheet) {
        workbook_close(workbook);
        throw std::runtime_error("Failed to add worksheet");
    }

    // Write headers
    if (!data.empty()) {
        int col = 0;
        for (const auto& field : data[0]) {
            worksheet_write_string(worksheet, 0, col, field.name(), NULL);
            col++;
        }
    }

    // Write data
    for (size_t row = 0; row < data.size(); ++row) {
        int col = 0;
        for (const auto& field : data[row]) {
            if (field.is_null()) {
                worksheet_write_string(worksheet, row + 1, col, "NULL", NULL);
            } else {
                worksheet_write_string(worksheet, row + 1, col, field.c_str(), NULL);
            }
            col++;
        }
    }

    lxw_error error = workbook_close(workbook);
    if (error) {
        throw std::runtime_error("Error in workbook_close: " + std::to_string(error));
    }
}

void generate_pdf(const std::vector<pqxx::row>& data, const std::string& filename) {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) {
        throw std::runtime_error("Failed to create PDF object");
    }

    HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetFontAndSize(page, font, 12);
    HPDF_Page_BeginText(page);
    HPDF_Page_MoveTextPos(page, 50, 750);

    HPDF_Page_ShowText(page, "Report");

    int ypos = 700;
    for (const auto& row : data) {
        std::string line;
        for (const auto& field : row) {
            line += field.as<std::string>() + " | ";
        }
        HPDF_Page_MoveTextPos(page, 0, -20);
        HPDF_Page_ShowText(page, line.c_str());
        ypos -= 20;
        if (ypos < 50) {
            HPDF_Page_EndText(page);
            page = HPDF_AddPage(pdf);
            HPDF_Page_SetFontAndSize(page, font, 12);
            HPDF_Page_BeginText(page);
            HPDF_Page_MoveTextPos(page, 50, 750);
            ypos = 700;
        }
    }

    HPDF_Page_EndText(page);
    HPDF_SaveToFile(pdf, filename.c_str());
    HPDF_Free(pdf);
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
    JWTAuth jwt_auth;

    CROW_ROUTE(app, "/")
    ([](){
        return "Hello, World!";
    });

    CROW_ROUTE(app, "/login").methods("POST"_method)
    ([&jwt_auth](const crow::request& req) {
        return jwt_auth.login(req);
    });

    CROW_ROUTE(app, "/create_table").methods(crow::HTTPMethod::POST)
([&jwt_auth](const crow::request& req) {  // Capture jwt_auth by reference
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
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
([&jwt_auth](const crow::request& req) {  // Capture jwt_auth by reference
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
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

CROW_ROUTE(app, "/read_all").methods(crow::HTTPMethod::GET)
([&jwt_auth](const crow::request& req) {  // Capture jwt_auth by reference
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
        auto tableName = req.url_params.get("table");
        if (!tableName) return crow::response(400, "Table name is required");
        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);
        pqxx::result R = W.exec("SELECT * FROM " + W.esc(tableName) + ";");

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
([&jwt_auth](const crow::request& req) {  // Capture jwt_auth by reference
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
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
([&jwt_auth](const crow::request& req) {  // Capture jwt_auth by reference
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
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

CROW_ROUTE(app, "/generate_pdf_by_id").methods(crow::HTTPMethod::GET)
([&jwt_auth](const crow::request& req) {  
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
        auto tableName = req.url_params.get("table");
        int startId = std::stoi(req.url_params.get("start_id"));
        int endId = std::stoi(req.url_params.get("end_id"));

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        auto data = get_data_by_id_range(W, tableName, startId, endId);

        std::string filename = "report_by_id_range.pdf";
        generate_pdf(data, filename);

        return crow::response(200, "PDF generated successfully: " + filename);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/generate_pdf_by_date").methods(crow::HTTPMethod::GET)
([&jwt_auth](const crow::request& req) {  
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
        auto tableName = req.url_params.get("table");
        std::string startDate = req.url_params.get("start_date");
        std::string endDate = req.url_params.get("end_date");

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        auto data = get_data_by_date_range(W, tableName, startDate, endDate);

        std::string filename = "report_by_date_range.pdf";
        generate_pdf(data, filename);

        return crow::response(200, "PDF generated successfully: " + filename);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/generate_excel_by_date").methods(crow::HTTPMethod::GET)
([&jwt_auth](const crow::request& req) {  
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
        auto tableName = req.url_params.get("table");
        std::string startDate = req.url_params.get("start_date");
        std::string endDate = req.url_params.get("end_date");

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        auto data = get_data_by_date_range(W, tableName, startDate, endDate);

        std::string filename = "report_by_date_range.xlsx";
        generate_excel(data, filename);

        return crow::response(200, "Excel file generated successfully: " + filename);
    } catch (const std::exception& e) {
        return crow::response(500, e.what());
    }
});

CROW_ROUTE(app, "/generate_excel_by_id").methods(crow::HTTPMethod::GET)
([&jwt_auth](const crow::request& req) {  
    if (!jwt_auth.protect_route(req)) {
        return crow::response(401, "Unauthorized");
    }
    try {
        auto tableName = req.url_params.get("table");
        int startId = std::stoi(req.url_params.get("start_id"));
        int endId = std::stoi(req.url_params.get("end_id"));

        pqxx::connection C("dbname=mydatabase user=nada password=catsforever hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        auto data = get_data_by_id_range(W, tableName, startId, endId);

        std::string filename = "report_by_id_range.xlsx";
        generate_excel(data, filename);

        return crow::response(200, "Excel file generated successfully: " + filename);
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


/*
g++ main.cpp -o output -I/home/ubuntu/project/code/include -lpqxx -lpq  -lssl -lcrypto -lhpdf -lxlsxwriter



//https://claude.ai/chat/21cdce26-88f3-41c5-a31e-74462e713db4
Certainly! Here's a README file that documents the data structures and API endpoints for your project:

```markdown
# Database API Documentation

This document outlines the API endpoints and data structures used in our database management system.

## API Endpoints

### 1. Login
- **URL**: `/`
- **Method**: POST
- **Data**:
  ```json
  {
    "username": "string",
    "password": "string"
  }
  ```
- **Response**: JWT token (string)

### 2. Create Table
- **URL**: `/create_table`
- **Method**: POST
- **Headers**: 
  - `Authorization: Bearer <JWT_TOKEN>`
  - `Content-Type: application/json`
- **Data**:
  ```json
  {
    "table_name": "string",
    "columns": ["column_name DATA_TYPE", ...]
  }
  ```
- **Example**:
  ```json
  {
    "table_name": "users",
    "columns": ["name TEXT", "email TEXT", "age INTEGER"]
  }
  ```

### 3. Insert Record
- **URL**: `/insert`
- **Method**: POST
- **Headers**: 
  - `Authorization: Bearer <JWT_TOKEN>`
  - `Content-Type: application/json`
- **Data**:
  ```json
  {
    "table_name": "string",
    "columns": {
      "column_name": "value",
      ...
    }
  }
  ```
- **Example**:
  ```json
  {
    "table_name": "users",
    "columns": {
      "name": "John Doe",
      "email": "john@example.com",
      "age": "30"
    }
  }
  ```

### 4. Read All Records
- **URL**: `/read_all?table=<table_name>`
- **Method**: GET
- **Headers**: 
  - `Authorization: Bearer <JWT_TOKEN>`
- **Response**: JSON array of records

### 5. Update Record
- **URL**: `/update`
- **Method**: POST
- **Headers**: 
  - `Authorization: Bearer <JWT_TOKEN>`
  - `Content-Type: application/json`
- **Data**:
  ```json
  {
    "table_name": "string",
    "columns": {
      "id": "string",
      "column_name": "new_value",
      ...
    }
  }
  ```
- **Example**:
  ```json
  {
    "table_name": "users",
    "columns": {
      "id": "1",
      "name": "Jane Doe",
      "email": "jane@example.com",
      "age": "31"
    }
  }
  ```

### 6. Delete Record
- **URL**: `/delete`
- **Method**: POST
- **Headers**: 
  - `Authorization: Bearer <JWT_TOKEN>`
  - `Content-Type: application/json`
- **Data**:
  ```json
  {
    "table_name": "string",
    "id": "string"
  }
  ```
- **Example**:
  ```json
  {
    "table_name": "users",
    "id": "1"
  }
  ```

## Notes

- All endpoints except Login require a valid JWT token in the Authorization header.
- The `id` field is automatically created for each table as a primary key.
- When creating a table, specify the column names and their data types (e.g., "name TEXT").
- For insert and update operations, provide the column names and their values.
- The update operation requires the `id` of the record to be updated.
- The delete operation requires the `id` of the record to be deleted.

## Error Handling

- If an operation fails, the API will return an appropriate HTTP status code along with an error message.
- Common error codes:
  - 400: Bad Request (invalid input)
  - 401: Unauthorized (invalid or missing token)
  - 404: Not Found (table or record not found)
  - 500: Internal Server Error

## Security

- Always use HTTPS in production to secure data transmission.
- Keep your JWT tokens secure and do not share them.
- Tokens have an expiration time; refresh or obtain a new token when needed.

```

This README provides a comprehensive guide to the API endpoints, data structures, and usage notes for your database management system. It covers all the main operations (Create, Read, Update, Delete) as well as table creation and authentication.

You can save this as a `README.md` file in your project repository. This documentation will help other developers understand how to use your API, what data structures to send, and what to expect in responses.


*/

/*
usage example of date and ip ranges:
pqxx::connection C("dbname=your_db user=your_user password=your_password");
pqxx::work W(C);

// Get data by date range
std::string startDate = "2024-01-01"; // format YYYY-MM-DD
std::string endDate = "2024-12-31";
std::vector<pqxx::row> dateData = get_data_by_date_range(W, "your_table_name", startDate, endDate);

// Get data by IP range
pqxx::connection C("dbname=your_db user=your_user password=your_password");
pqxx::work W(C);

// Get data by ID range
int startId = 1;  // Start ID for the range
int endId = 10;   // End ID for the range
std::vector<pqxx::row> idData = get_data_by_id_range(W, "your_table_name", startId, endId);

// Process the retrieved data
for (const auto& row : idData) {
    // Example: Print the data from each row
    std::cout << "ID: " << row["id"].as<int>() << ", Created At: " << row["created_at"].as<std::string>() << std::endl;
}

W.commit();

*/

/*

curl -X POST http://localhost:18080/login \
  -H "Content-Type: application/json" \
  -d '{"username": "your_username", "password": "secret_password"}' \

curl -X POST http://localhost:18080/create_table \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)" \
  -H "Content-Type: application/json" \
  -d '{
    "table_name": "my_table",
    "columns": [
      "name VARCHAR(100)",
      "age INT",
      "email VARCHAR(100)"
    ]
  }'


curl -X POST http://localhost:18080/insert \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)" \
  -H "Content-Type: application/json" \
  -d '{
    "table_name": "my_table",
    "columns": {
      "name": "John Doe",
      "age": 30,
      "email": "john.doe@example.com"
    }
  }'


curl -X GET "http://localhost:18080/generate_pdf_by_date?table=my_table&start_date=2024-08-31&end_date=2024-08-31" \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)"

curl -X GET "http://localhost:18080/generate_pdf_by_id?table=my_table&start_id=1&end_id=2" \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)"


curl -X GET "http://localhost:18080/generate_excel_by_date?table=my_table&start_date=2024-08-31&end_date=2024-08-31" \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)"

curl -X GET "http://localhost:18080/generate_excel_by_id?table=my_table&start_id=1&end_id=2" \
  -H "Authorization: Bearer $(cat /home/ubuntu/project/code/jwt_token.txt)"


*/