# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.8)

# Name of the project
project(preimage)

set(PRINT_DEBUG false)

add_definitions(-std=c++17 -Wall -Werror -O3)

if(PRINT_DEBUG)
  add_definitions(-DPRINT_DEBUG)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

if(NOT TARGET spdlog)
  # Stand-alone build
  find_package(spdlog REQUIRED)
endif()

# CryptoMiniSAT
find_package(cryptominisat5 REQUIRED)

# Crypto++
find_package(CryptoPP REQUIRED)

# Google or-tools
find_package(ortools CONFIG REQUIRED)

# Eigen
find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
set(EIGEN3_LIBRARIES ${EIGEN3_LIBRARIES})
include_directories(${EIGEN3_INCLUDE_DIR})

# Add all the source files needed to build the executable
add_executable(preimage
               src/main.cpp
               src/problem_instance.cpp
               src/core/config.cpp
               src/core/bit.cpp
               src/core/factor.cpp
               src/core/sym_bit_vec.cpp
               src/core/solver.cpp
               src/core/sym_hash.cpp
               src/hashing/hash_funcs.cpp
               src/hashing/sym_sha256.cpp
               src/hashing/sym_md5.cpp
               src/hashing/sym_ripemd160.cpp
               src/cmsat/cmsat_solver.cpp
               src/bp/node.cpp
               src/bp/graph.cpp
               src/bp/bp_solver.cpp
               src/ortools_cp/cp_solver.cpp)

target_link_libraries(preimage PRIVATE spdlog::spdlog)
target_link_libraries(preimage PUBLIC cryptominisat5)
target_link_libraries(preimage PUBLIC cryptopp-shared)
target_link_libraries(preimage PUBLIC ortools::ortools)
