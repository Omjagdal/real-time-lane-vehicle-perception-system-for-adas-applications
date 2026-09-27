# ─────────────────────────────────────────────────────────────────────
#  ADAS Perception Pipeline — C++17 Backend
#  Build Configuration
# ─────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.14)
project(ADAS_Perception_CPP
    VERSION     2.0.0
    DESCRIPTION "Real-time ADAS perception pipeline with YOLO detection, IoU tracking, and TTC-based FCW"
    LANGUAGES   CXX
)

# ── C++ Standard ──────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build Type (default: Release for production performance) ──────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler Warnings (strict, production-grade) ──────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wcast-align
    -Wunused
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
    -Wno-sign-compare          # Suppress in OpenCV interop (size_t vs int)
    -Wno-unused-parameter      # Suppress for interface conformance
)

# ── Optimization Flags ────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG -march=native)
    message(STATUS "[ADAS] Release build: -O3 -march=native")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g -fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "[ADAS] Debug build: ASan + UBSan enabled")
endif()

# ── Dependencies ──────────────────────────────────────────────────────
find_package(OpenCV 4.8 REQUIRED COMPONENTS core imgproc imgcodecs videoio video dnn highgui)
message(STATUS "[ADAS] OpenCV ${OpenCV_VERSION} found: ${OpenCV_INCLUDE_DIRS}")

# ── Source Files ──────────────────────────────────────────────────────
set(ADAS_SOURCES
    src/main.cpp
    src/preprocessing.cpp
    src/lane_detection.cpp
    src/vehicle_detection.cpp
    src/estimators.cpp
    src/tracker.cpp
    src/visualization.cpp
)

# ── Target Definition ─────────────────────────────────────────────────
add_executable(adas_server ${ADAS_SOURCES})

target_include_directories(adas_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(adas_server PRIVATE
    ${OpenCV_LIBS}
    pthread
)

# ── Build Summary ─────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "════════════════════════════════════════════════════")
message(STATUS "  ADAS Perception Pipeline v${PROJECT_VERSION}")
message(STATUS "  Build Type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Std    : C++${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenCV     : ${OpenCV_VERSION}")
message(STATUS "════════════════════════════════════════════════════")
message(STATUS "")
