# OpenCV_Py_from_CPP

## Programming side:

1. Obviously, make your cpp and python program.

Example for CMAKE environment:

```cmake
# Make a normal C++ build environment
cmake_minimum_required(VERSION 3.10)
project(pyCpp1)

# Find the Python interpreter and libraries
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

# Add foo.py to our build
configure_file(foo.py foo.py COPYONLY)

add_executable(pyCpp1 main.cpp)

# Link the Python libraries
target_link_libraries(pyCpp1 ${PYTHON_LIBRARIES})

```

2.) Install python3-dev into your environment.

```bash
$ sudo apt update && sudo apt install -y python3-dev
```


FYI, my library ends up being "/usr/include/python3.10/Python3.h", in Ubuntu 22.04

3.) Programming

Basic example (including CMakeText and code) can be found here: https://github.com/jbates35/Basic_Py_from_CPP


Note. We need to send an OpenCV Mat, and get a series of Rects in return. It might not be particularly easy to get the rects as a return, what might be easier is to get an array of array of ints. (i.e., something like int returnVal[30][4])

*Note: This will likely have to be a class. The constructor of the class will run:*
```cpp
    //Initialize the Python Interpreter
    Py_Initialize();

    PyRun_SimpleString("import os");
    PyRun_SimpleString("import cv2 as cv");
    PyRun_SimpleString("import numpy as np");
```
*and whatever else needs to be imported. There will probably have to be some adjustment for*

-----------


## Install side:

1.) Follow these steps (From https://coral.ai/docs/accelerator/get-started/):

```bash
$ echo "deb https://packages.cloud.google.com/apt coral-edgetpu-stable main" | sudo tee /etc/apt/sources.list.d/coral-edgetpu.list
$ curl https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
$ sudo apt-get update
$ sudo apt-get install libedgetpu1-std
$ sudo apt-get install python3-pycoral
```

Try the example to make sure it works

2.) Next, make sure python libraries are installed

```bash
$ sudo apt update
$ sudo apt install -y python3-pip
$ python3 -m pip install opencv-python==4.5.5.64
$ python3 -m pip install opencv-contrib-python==4.5.5.64 
```

*Note, on Ubuntu at least, I had to install:*
```bash
$ sudo apt-get install libgl1-mesa-glx
```

