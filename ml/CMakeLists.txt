# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.8)

# Name of the project
project(hash_reversal)

add_definitions(-std=c++17 -Wall -Werror -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp REQUIRED)
find_package(GTSAM REQUIRED)
include_directories(include ${GTSAM_INCLUDE_DIR})
set(GTSAM_LIBRARIES gtsam)

find_package(GTSAMCMakeTools)
include(GtsamMakeConfigFile)
include(GtsamBuildTypes)

# Add all the source files needed to build the executable
add_executable(hash_reversal
               src/main.cpp
               src/utils/config.cpp
               src/hash_reversal/factor.cpp
               src/hash_reversal/bayes_net.cpp
               src/hash_reversal/dataset.cpp
               src/hash_reversal/factor_graph.cpp
               src/hash_reversal/inference_tool.cpp
               src/hash_reversal/probability.cpp)

target_link_libraries(hash_reversal gtsam yaml-cpp)
