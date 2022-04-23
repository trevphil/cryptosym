# Minimum version of CMake required to build this project
cmake_minimum_required(VERSION 3.8)

# Name of the project
project(preimage)

set(BUILD_TESTS ON)

add_definitions(-std=c++17 -Wshadow -Wunused -Wall -Werror -O3)
# add_compile_options(-g)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Boost
find_package(Boost REQUIRED)

# CryptoMiniSAT
find_package(cryptominisat5 REQUIRED)

# Crypto++
if(BUILD_TESTS)
  find_package(CryptoPP REQUIRED)
endif()

# Eigen
find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
set(EIGEN3_LIBRARIES ${EIGEN3_LIBRARIES})
include_directories(${EIGEN3_INCLUDE_DIR})

# Add all the source files needed to build the executable
add_library(preimage SHARED
            src/problem_instance.cpp
            src/core/config.cpp
            src/core/bit.cpp
            src/core/cnf.cpp
            src/core/logic_gate.cpp
            src/core/sym_bit_vec.cpp
            src/core/solver.cpp
            src/core/sym_hash.cpp
            src/hashing/sym_sha256.cpp
            src/hashing/sym_md5.cpp
            src/hashing/sym_ripemd160.cpp
            src/cmsat/cmsat_solver.cpp
            src/preimage_sat/preimage_sat.cpp
            src/bp/node.cpp
            src/bp/graph.cpp
            src/bp/bp_solver.cpp)

target_link_libraries(preimage Boost::boost)
target_link_libraries(preimage cryptominisat5)

add_executable(main src/main.cpp)
target_link_libraries(main preimage)

if(BUILD_TESTS)
  add_executable(unit_tests tests/unit_tests.cpp)
  target_link_libraries(unit_tests preimage)
  target_link_libraries(unit_tests cryptopp-shared)
endif()
