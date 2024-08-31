#include <iostream>
#include "crow_all.h"
using namespace std;
/*
this is a simple root API that prints hello world
run with: g++ main_r.cpp -o output
then: ./output

test using:
http://0.0.0.0:18080/
*/


int main() {

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([](){
        return "Hello, World!";
    });

     app.port(18080).multithreaded().run();
    
    return 0;
}