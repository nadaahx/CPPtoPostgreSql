# Overview
This project provides a C++ web service that interacts with a PostgreSQL database. The service allows users to create tables, insert data, update records, read all records, and delete records through HTTP requests using the Crow web framework.

## Dependencies
Before running the project, ensure you have the following dependencies installed:
- **C++ Compiler**: Install a C++ compiler like `g++` or `clang++`. For example, on Ubuntu, you can install `g++` using:
  ```sh
  sudo apt-get update
  sudo apt-get install g++
  ```
- **PostgreSQL and libpqxx**: Install PostgreSQL and the libpqxx library for C++ to interact with PostgreSQL. On Ubuntu, you can use:
 ```sh
 sudo apt-get update
sudo apt-get install postgresql libpqxx-dev libasio-dev
sudo apt-get inatsll libxlsxwriter-dev
sudo apt-get install libhpdf-dev
 ```
- **Crow Web Framework**: header is provided in the repo
- **Install OpenSSL**:
  ```sh
  sudo apt-get install libssl-dev
  ```
- **Install nlohmann-json**: This JSON library is a dependency for jwt-cpp.
```sh
sudo apt-get install nlohmann-json3-dev
```
- **Clone the jwt-cpp repository**:
```sh
git clone https://github.com/Thalhammer/jwt-cpp.git
```
- **Navigate to the jwt-cpp directory**:
```sh
cd jwt-cpp
```
- **Build the project using CMake or copy the headers directly to your project.**
- **Update the linker cache:**
```sh
sudo ldconfig
```

   ## Creating a User and Database

   To create a user and a database in PostgreSQL, follow these steps:

   1. Access the PostgreSQL command line as the `postgres` user:
      ```
      sudo su - postgres
      psql
      ```

   2. Create a new user:
      ```
      CREATE USER username WITH PASSWORD 'password';
      ```

      Replace `username` with the desired username and `'password'` with the desired password.

   3. Create a new database:
      ```
      CREATE DATABASE database_name OWNER username;
      ```

      Replace `database_name` with the desired database name and `username` with the username of the user who will own the database.

   5. Exit from the `postgres` user shell:
      ```
      \q
      exit
      ```


## Functions

- `split(const std::string& s, char delimiter)`: Splits a string by a specified delimiter and returns a vector of tokens.

- `extract_number(const std::string& column_name)`: Extracts and returns the number found in a column name.

- `get_id_from_columns(const std::string& json_data)`: Extracts the ID from the column names in a JSON string.

- `parseTableSchema(const std::string& schema, std::string& tableName, std::vector<std::string>& columns)`: Parses the table schema from a JSON-like string to extract the table name and columns.

- `join(const std::vector<std::string>& elements, const std::string& delimiter)`: Joins a vector of strings into a single string with a specified delimiter.

- `create_table(pqxx::work& work, const std::string& table_name, Columns&&... columns)`: Creates a table in the database with specified columns.

- `parse_column_value(const std::string& column_value)`: Parses a "column value" string into a tuple containing the column and value.

- `insert(pqxx::work& W, const std::string& table, Args... args)`: Inserts values into a specified table.

- `update(pqxx::work& W, const std::string& table, std::string id, T&&... ts)`: Updates a record in a specified table based on its ID.

- `delete_record(pqxx::work& W, const std::string& table, std::string id)`: Deletes a record from a specified table based on its ID.

- `read_all_rows(pqxx::work& W, const std::string& table)`: Reads and prints all rows from a specified table.

## Endpoints

### /create_table (POST)

Creates a new table with the specified schema.

**Request Body Format:**

  ```json
  {
    "table_name": "string",
    "columns": ["column_name DATA_TYPE", ...]
  }
  ```

**Example:**

  ```json
  {
    "table_name": "users",
    "columns": ["name TEXT", "email TEXT", "age INTEGER"]
  }
  ```

### /insert (POST)

Inserts a new record into the specified table.

**Request Body Format:**

  ```json
  {
    "table_name": "string",
    "columns": {
      "column_name": "value",
      ...
    }
  }
  ```

**Example:**

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

### /read_all (GET)

Reads all rows from the specified table.

**Query Parameter:**

- `table`: The name of the table to read from.

**Example:**

```sql
GET /read_all?table=students
```

### /update (POST)

Updates a record in the specified table. The record is identified by the id column.

**Request Body Format:**

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

**Example:**

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

### /delete (POST)

Deletes a record from the specified table.

**Request Body Format:**

  ```json
  {
    "table_name": "users",
    "id": "1"
  }
  ```



## Compiling and Running C++ Code

1. Compile the C++ code with `g++`:
  ```sh
   g++ main.cpp -o output -lpqxx -lpq -lssl -lcrypto
   ```
You may need to specify the path to headers 
```sh
g++ main.cpp -o output -I/path/to/headers -lpqxx -lpq -lssl -lcrypto
```

2. Run the compiled executable:
 ```
   ./output
   ```
## Testing
```sh
curl -X POST http://localhost:18080/ -H "Content-Type: application/json" -d '{"username": "test","password": "testpass" }'
```

```sh
curl -X GET "http://localhost:18080/read_all?table=test" -H "Authorization: Bearer -token generated-"
```




