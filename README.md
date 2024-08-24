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
 ```
- **Crow Web Framework**: header is provided in the repo

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
   "table_name": "your_table_name",
   "columns": ["column1 dataType", "column2 dataType", "columnN dataType"]
}
```

**Example:**

```json
{
   "table_name": "students",
   "columns": ["id SERIAL PRIMARY KEY", "name VARCHAR(100)", "age INT"]
}
```

### /insert (POST)

Inserts a new record into the specified table.

**Request Body Format:**

```json
{
   "table_name": "your_table_name",
   "columns": ["column_name value", "column_name value", ...]
}
```

**Example:**

```json
{
   "table_name": "students",
   "columns": ["name John", "age 20"]
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
   "table_name": "your_table_name",
   "columns": ["column_name value", "column_name value", ...]
}
```

**Example:**

```json
{
   "table_name": "students",
   "columns": ["id 1", "name Jane", "age 22"]
}
```

### /delete (POST)

Deletes a record from the specified table.

**Request Body Format:**

```json
{
   "table_name": "your_table_name",
   "id": "record_id"
}
```

**Example:**

```json
{
   "table_name": "students",
   "id": "1"
}
```



## Compiling and Running C++ Code

1. Compile the C++ code with `g++`:
  ```
   g++ main.cpp -o output -lpqxx -lpq
   ```


2. Run the compiled executable:
 ```
   ./output
   ```



