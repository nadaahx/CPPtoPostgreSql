#include <Python.h>
#include <iostream>

int main() {
    // Initialize the Python Interpreter
    Py_Initialize();

    // Define the Python script to run
    const char* scriptPath = "jojo.py";

    // Run the Python script
    FILE* file = fopen(scriptPath, "r");
    if (file != NULL) {
        PyRun_SimpleFile(file, scriptPath);
        fclose(file);
    } else {
        std::cerr << "Failed to open script: " << scriptPath << std::endl;
    }

    // Finalize the Python Interpreter
    Py_Finalize();

    return 0;
}