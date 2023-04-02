#include "fishMLWrapper.h"

FishMLWrapper::FishMLWrapper()
{
}

FishMLWrapper::~FishMLWrapper()
{
   //Clean up python interpreter
   Py_DECREF(_pFunc);
   Py_DECREF(_pModule);
   //Py_Finalize();
}

int FishMLWrapper::init()
{
   //Store needed python imports/commands to be parsed through
   _pyCommands = {
      "import sys",
      "import cv2 as cv",
      "import numpy as np",
      "face_cascade = cv.CascadeClassifier('haarcascade_frontalface_default.xml')"
   };

   //Initialize python interpreter
   Py_Initialize();

   //Parse through python commands
   for (string command : _pyCommands)
   {
      PyRun_SimpleString(command.c_str());
   }

   if (_import_array() < 0) 
   {
      PyErr_Print(); 
      PyErr_SetString(PyExc_ImportError, "numpy.core.multiarray failed to import");
   }
  
   //Set folder path to python script
   PySys_SetPath(L".");

   //Import python script
   _pModule = PyImport_ImportModule("fishML");

   //Make sure python script was imported
   if (!_pModule)
   {
      PyErr_Print();
      return -1;
   }

   //Get python function from script
   _pFunc = PyObject_GetAttrString(_pModule, "fishML");

   //Make sure python function was imported
   if (!_pFunc || !PyCallable_Check(_pFunc))
   {
      PyErr_Print();
      return -1;
   }

   return 1;
}

int FishMLWrapper::update(Mat &srcImg, vector<Rect> &ROIs)
{
   if(srcImg.empty())
   {
      cout << "No source image for update" << endl;
      return -1;
   }

   //Convert Mat to numpy array
   npy_intp dims[3] = { srcImg. rows, srcImg.cols, srcImg.channels() };
   PyObject* pImg = PyArray_SimpleNewFromData(srcImg.dims + 1, (npy_intp*) & dims, NPY_UINT8, srcImg.data);

   //Create python argument list
   PyObject* pArgs = PyTuple_New(1);
   PyTuple_SetItem(pArgs, 0, pImg);

   //Create return value pointer
   PyObject* pReturn = PyObject_CallObject(_pFunc, pArgs);

   //Free memory of arguments
   Py_DECREF(pImg);
   Py_DECREF(pArgs);

   //Make sure there's a return value
   if (pReturn == NULL)
   {
      PyErr_Print();
      Py_DECREF(pReturn);
      return -1;
   }

   //Parse return value to vector of Rects
   //First declare variables needed
   vector<Rect> tempROIs;
   PyObject* pROI;
   PyObject* pVal;

   //Get size of return value
   Py_ssize_t rSize = PyList_Size(pReturn);

   //Iterate through return value
   for (Py_ssize_t i = 0; i < rSize; i++)
   {
      //This stores the iterated items which will form the rects later
      pROI = PyList_GetItem(pReturn, i);
      vector<int> rectVals;

      //Parse through to get the rect coordinates
      for (Py_ssize_t j = 0; j < 4; j++)
      {
         // Get object of value, convert it to a c++ style int, and store it in the vector
         pVal = PyList_GetItem(pROI, j);
         int val = PyLong_AsLong(pVal);
         rectVals.push_back(val);
      }

      //Create Rect from vector and dump for later
      Rect ROI(rectVals[0], rectVals[1], rectVals[2], rectVals[3]);
      tempROIs.push_back(ROI);
   }

   //Free memory of return value
   Py_DECREF(pROI);
   Py_DECREF(pVal);
   Py_DECREF(pReturn);

   //Clear ROIs and store new ROIs
   ROIs.clear();

   //If no ROIs were found, return 0
   if(tempROIs.size() == 1 && tempROIs[0].x == -1)
   {
      return 0;
   }

   //Success, dump ROIs
   ROIs = tempROIs;

   return 1;
}
