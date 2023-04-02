#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <Python.h>
#include <numpy/arrayobject.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

using namespace std;
using namespace cv;

class FishMLWrapper
{
public:
   FishMLWrapper();
   ~FishMLWrapper();

   int init();
   int update(Mat &srcImg, vector<Rect> &ROIs);

private:
   vector<string> _pyCommands;
   PyObject* _pModule;
   PyObject* _pFunc;
};