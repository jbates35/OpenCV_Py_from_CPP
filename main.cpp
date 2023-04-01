#include <iostream>
#include <Python.h>

int main(int argc, char *argv[]) {

    //Initialize the Python Interpreter
    Py_Initialize();

    PyRun_SimpleString("import os");
    PyRun_SimpleString("import cv2 as cv");
    PyRun_SimpleString("import numpy as np");

    //Set folder for directory of python module
    PySys_SetPath(L".");

    // Add python module
    PyObject *pModule = PyImport_ImportModule("fishML");

    //Make sure module could be found
    if (pModule == NULL) {
        PyErr_Print();
        return 1;
    }

    //Get function from module
    PyObject *pFunc = PyObject_GetAttrString(pModule, "fishML");

    // Make sure function could be found
    if(PyErr_Occurred() || !PyCallable_Check(pFunc)) {
        PyErr_Print();
        return 1;
    }

    //Create arguments for function (should be nothing)
    PyObject *pArgs = PyTuple_New(0);

    //Create return value pointer
    PyObject *pReturn = PyObject_CallObject(pFunc, pArgs);

    //Make sure return value is not null
    if (pReturn == NULL) {
        PyErr_Print();
        return 1;
    }

    //Free memory
    Py_DECREF(pReturn);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    Py_Finalize();

    return 0;
}