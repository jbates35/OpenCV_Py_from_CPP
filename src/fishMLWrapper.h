#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <mutex>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

using namespace std;
using namespace cv;

//Definitions
namespace fml
{
   const string _modelPath = "'models/fishModel1'";
   const string _labelPath = "'labels/mscoco_label_map.pbtxt.txt'";
}

using namespace fml;

//Data structure for rect, id, score
struct FishMLData
{
   Rect ROI;
   double score;
};

class FishMLWrapper
{
public:
   FishMLWrapper();
   ~FishMLWrapper();

   int init();
   int update(Mat &srcImg, vector<FishMLData> &objData);

private:
   PyObject* _pModule;
   PyObject* _pFunc;
   PyObject* _pReturn;
   PyObject* _pROI;
   PyObject* _pArgs;
   PyObject* _pVal;
   PyObject* _pImg;
};