# PostgreSQL Command Line Guide

## Opening PostgreSQL Terminal

1. Switch to the postgres user:
   ```
   sudo su - postgres
   ```

2. Access the PostgreSQL command line:
   ```
   psql
   ```

## Connecting to a Database

 Connect to the database `mydb` as user `jojo` on `localhost`:
   ```
   psql -U username -d database -h 127.0.0.1
   ```

   - `-U `: Specifies the username.
   - `-d `: Specifies the database name.
   - `-h 127.0.0.1`: Specifies the host address (localhost).

## Managing Tables

### Dropping a Table

 Drop a table if it exists:
   ```
   DROP TABLE IF EXISTS tablename;
   ```

   Replace `tablename` with the name of the table you want to drop.

### Creating a Table


```
Create a new table:
   CREATE TABLE tablename (
       column1 datatype,
       column2 datatype,
       ...
   );
   ```


   Replace `tablename` with the desired table name and specify the columns with their respective data types.

## Quitting the PostgreSQL Command Line

Quit the PostgreSQL command line interface:
  ``` 
  \q 
  ```

## Exiting the `postgres` User Shell

Exit from the `postgres` user shell:
   ```
   exit
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
----------------------------------------
WHEN TESING WITH CURL BE CAREFUL OF THE STRUCTURE OF THE REQUEST'S BODY


