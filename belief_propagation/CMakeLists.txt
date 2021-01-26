# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.8)

# Name of the project
project(belief_propagation)

add_definitions(-std=c++17 -Wall -Werror -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(yaml-cpp REQUIRED)
if(NOT TARGET spdlog)
  # Stand-alone build
  find_package(spdlog REQUIRED)
endif()
include_directories(include)

# Add all the source files needed to build the executable
add_executable(belief_propagation
               src/main.cpp
               src/utils/config.cpp
               src/belief_propagation/factor.cpp
               src/belief_propagation/dataset.cpp
               src/belief_propagation/factor_graph.cpp
               src/belief_propagation/inference_tool.cpp
               src/belief_propagation/probability.cpp)

target_link_libraries(belief_propagation PRIVATE yaml-cpp spdlog::spdlog)
