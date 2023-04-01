# Make a normal C++ build environment
cmake_minimum_required(VERSION 3.10)

#Require C++17
set(CMAKE_CXX_STANDARD 17)
project(ocvPyCpp)

# Add fishML to our build
configure_file(py/fishML.py fishML.py COPYONLY)
add_executable(ocvPyCpp main.cpp)

# Find the Python interpreter and libraries
find_package(PythonLibs REQUIRED)
find_package(OpenCV REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

# Link the Python libraries
target_link_libraries(ocvPyCpp ${PYTHON_LIBRARIES} ${OpenCV_LIBS})