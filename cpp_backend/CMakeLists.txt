cmake_minimum_required(VERSION 3.14)
project(ADAS_Perception_CPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# Source files (we will add them as we build them)
set(SOURCES
    src/main.cpp
    src/preprocessing.cpp
    src/lane_detection.cpp
    src/vehicle_detection.cpp
    src/estimators.cpp
    src/tracker.cpp
    src/visualization.cpp
)

# Build executable
add_executable(adas_server ${SOURCES})

# Link libraries
target_link_libraries(adas_server ${OpenCV_LIBS} pthread)
