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
      "import os",
      "from pycoral.adapters import common",
      "from pycoral.adapters import detect",
      "from pycoral.utils.edgetpu import make_interpreter",
      "script_dir = pathlib.Path(__file__).parent.absolute()",
      "model_file = os.path.join(script_dir, '" + _modelPath + "')",
      "interpreter = make_interpreter(model_file)",
      "interpreter.allocate_tensors()"
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

int FishMLWrapper::update(Mat &srcImg, vector<FishMLData> &objData)
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
   vector<FishMLData> tempObjData;
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

      //Get ID
      pVal = PyList_GetItem(pROI, 4);
      double score = PyFloat_AsDouble(pVal);

      //Create Rect from vector and dump for later
      Rect ROI(rectVals[0], rectVals[1], rectVals[2], rectVals[3]);

      FishMLData data = { ROI, score };

      tempObjData.push_back(data);
   }

   //Free memory of return value
   Py_DECREF(pROI);
   Py_DECREF(pVal);
   Py_DECREF(pReturn);

   //Clear ROIs and store new ROIs
   objData.clear();

   //If no ROIs were found, return 0
   if(tempObjData.size() == 1 && tempObjData[0].ROI.x == -1)
   {
      return 0;
   }

   //Success, dump ROIs
   objData = tempObjData;

   return 1;
}
