#include <iostream>
#include <pqxx/pqxx>
using namespace std;

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
        pqxx::result R = W.exec("SELECT version();");

        cout << "PostgreSQL version: " << R[0][0].c_str() << endl;

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

    return 0;
}
