# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.8)

# Name of the project
project(dataset_generator)

add_definitions(-std=c++17 -Wall -Werror -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

if(NOT TARGET spdlog)
  # Stand-alone build
  find_package(spdlog REQUIRED)
endif()

find_package(cryptominisat5 REQUIRED)

# Add all the source files needed to build the executable
add_executable(dataset_generator
               src/main.cpp
               src/bit.cpp
               src/factor.cpp
               src/sym_bit_vec.cpp
               src/sym_hash.cpp
               src/hash_funcs.cpp
               src/sha256.cpp
               src/solver.cpp
               src/cmsat_solver.cpp)

target_link_libraries(dataset_generator PRIVATE spdlog::spdlog)
target_link_libraries(dataset_generator PUBLIC cryptominisat5)
