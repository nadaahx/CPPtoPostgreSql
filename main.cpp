#include <iostream>
#include <pqxx/pqxx>
using namespace std;

template<typename T>
void create(pqxx::work &W, const string &table, const string &column, const T &value) {
    W.exec0("INSERT INTO " + table + " (" + column + ") VALUES (" + W.quote(value) + ");");
    W.commit();
    cout << "Inserted into " << table << ": " << column << " = " << value << endl;
}

template<typename T>
void update(pqxx::work &W, const string &table, const string &column, int id, const T &new_value) {
    W.exec0("UPDATE " + table + " SET " + column + " = " + W.quote(new_value) + " WHERE id = " + W.quote(to_string(id)) + ";");
    W.commit();
    cout << "Updated in " << table << ": ID " << id << " set " << column << " to " << new_value << endl;
}

template<typename T>
void delete_record(pqxx::work &W, const string &table, int id) {
    W.exec0("DELETE FROM " + table + " WHERE id = " + W.quote(to_string(id)) + ";");
    W.commit();
    cout << "Deleted from " << table << ": ID " << id << endl;
}

void read(pqxx::work &W, const string &table) {
    pqxx::result R = W.exec("SELECT * FROM " + table + ";");
    for (const auto &row : R) {
        for (const auto &field : row) {
            cout << field.c_str() << " ";
        }
        cout << endl;
    }
}

int main() {
    try {
        pqxx::connection C("dbname=mydatabase user=myuser password=mypassword hostaddr=127.0.0.1 port=5432");

        if (C.is_open()) {
            cout << "Connected to " << C.dbname() << endl;
        } else {
            cerr << "Failed to connect to database" << endl;
            return 1;
        }

        pqxx::work W(C);

        create<string>(W, "people", "name", "Alice");
        read(W, "people");
        update<string>(W, "people", "name", 1, "Bob");
        read(W, "people");
        delete_record<string> (W, "people", 1);
        read(W, "people");

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

