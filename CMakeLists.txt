# Make a normal C++ build environment
cmake_minimum_required(VERSION 3.10)

#Require C++17
set(CMAKE_CXX_STANDARD 17)
project(ocvPyCpp)

# include directories for src files
include_directories(src)
include_directories(/home/jbates/.local/lib/python3.10/site-packages/numpy/core/include/)

# Find the Python interpreter and libraries
find_package(PythonLibs REQUIRED)
find_package (Python3 COMPONENTS Interpreter NumPy)
find_package(OpenCV REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

# Add fishML to our build
configure_file(py/fishML.py fishML.py COPYONLY)
add_executable(ocvPyCpp src/main.cpp src/fishMLWrapper.cpp src/fishMLWrapper.h)

# Link the Python libraries
target_link_libraries(ocvPyCpp ${PYTHON_LIBRARIES} ${NUMPY_LIBRARIES} ${OpenCV_LIBS})