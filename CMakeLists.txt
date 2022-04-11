cmake_minimum_required(VERSION 3.5.1)
project(sort-tracker)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PythonExtensions REQUIRED)
find_package(NumPy REQUIRED)
include_directories(${NumPy_INCLUDE_DIRS})

# Try to find OpenCV
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
    # If the package has been found, several variables will
    # be set, you can find the full list with descriptions
    # in the OpenCVConfig.cmake file.
    # Print some message showing some of them
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_libraries(${OpenCV_LIBRARIES})
    message(STATUS "OpenCV library status:")
    message(STATUS "    version: ${OpenCV_VERSION}")
    message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
else ()
    message(FATAL_ERROR "Could not locate OpenCV")
endif()

# Try to find Eigen headers
find_package(Eigen3 REQUIRED)
if(Eigen3_FOUND)
    include( ${EIGEN3_USE_FILE} )
    message(STATUS "Eigen3 library status:")
    message(STATUS "    version: ${Eigen3_VERSION}")
    message(STATUS "    include path: ${Eigen3_INCLUDE_DIRS}")
else ()
    message(FATAL_ERROR "Could not locate Eigen3")
endif()

include_directories(${PROJECT_SOURCE_DIR}/sort/include)

file(GLOB SOURCE_FILES sort/src/*.cpp)
add_library(_sort MODULE sort/src/Py_SORT.cpp sort/src/tracker.cpp sort/src/track.cpp sort/src/munkres.cpp sort/src/kalman_filter.cpp)
python_extension_module(_sort)
install(TARGETS _sort LIBRARY DESTINATION sort)