# Make a normal C++ build environment
cmake_minimum_required(VERSION 3.10)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_BUILD_TYPE Release)

project(ocvPyCpp)

# Include directories for src files
include_directories(src)
include_directories(/home/dev/.local/lib/python3.10/site-packages/numpy/core/include/)

# Find the Python interpreter and libraries
find_package(PythonLibs REQUIRED)
find_package(Python3 COMPONENTS Interpreter NumPy)
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

# Add these settings for performance optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native")

include_directories(${PYTHON_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

# Add fishML to our build
configure_file(py/fishML.py fishML.py COPYONLY)
add_executable(ocvPyCpp src/main.cpp src/fishMLWrapper.cpp src/fishMLWrapper.h src/fishMLBase.cpp src/fishMLBase.h src/helperfunc.cpp src/helperfunc.h src/fishTracker.cpp src/fishTracker.h)

# Link the Python libraries
target_link_libraries(ocvPyCpp pthread ${PYTHON_LIBRARIES} ${NUMPY_LIBRARIES} ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT})
