#include <iostream>
#include <opencv2/opencv.hpp>

#include <Python.h>
#include <numpy/arrayobject.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {

    const cv::Mat srcimg = imread("img/arnie.png");
    
    //Initialize the Python Interpreter
    Py_Initialize();
    import_array();

    PyRun_SimpleString("import os");
    PyRun_SimpleString("import cv2 as cv");
    PyRun_SimpleString("import numpy as np");
    PyRun_SimpleString("face_cascade = cv.CascadeClassifier('haarcascade_frontalface_default.xml')");

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

    npy_intp npy_send[3] = {srcimg.rows, srcimg.cols, srcimg.channels()};

    //Create arguments for function (mat)
    npy_intp dims[3] = { srcimg.rows, srcimg.cols, srcimg.channels() };
    PyObject* pImg = PyArray_SimpleNewFromData(srcimg.dims + 1, (npy_intp*) &dims, NPY_UINT8, srcimg.data);

    PyObject *pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, pImg);
    
    //Create return value pointer
    PyObject *pReturn = PyObject_CallObject(pFunc, pArgs);

    //Make sure return value is not null
    if (pReturn == NULL) {
        PyErr_Print();
        return 1;
    }

    
    cout << "HERE?";
    

    //Convert the return value to a vector of vector of ints
    vector<Rect> ROIs;
    Py_ssize_t size = PyList_Size(pReturn);

    //Parse through return vector and make a vector of Rects
    for (Py_ssize_t i = 0; i < size; i++) {
        PyObject *pROI = PyList_GetItem(pReturn, i);
        vector<int> ROI_vect;
        for (Py_ssize_t j = 0; j < 4; j++) {
            PyObject *pVal = PyList_GetItem(pROI, j);
            ROI_vect.push_back(PyLong_AsLong(pVal));
        }
        Rect ROI(ROI_vect[0], ROI_vect[1], ROI_vect[2], ROI_vect[3]);
        ROIs.push_back(ROI);
    }

    //Free memory
    Py_DECREF(pReturn);
    Py_DECREF(pImg);
    Py_DECREF(pArgs);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);

    Py_Finalize();

    //Add rectangles to image
    for (auto rect : ROIs) 
    {
        rectangle(srcimg, rect, Scalar(0, 255, 255), 2);
    }

    char keyPress = NULL;
    
    while(keyPress != 'q')
    {
        imshow("Arnie", srcimg);
        keyPress = waitKey(0);
    }

    return 0;
}