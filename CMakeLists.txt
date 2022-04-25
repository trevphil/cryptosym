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

if(BUILD_TESTS)
  # Crypto++
  find_package(CryptoPP REQUIRED)

  # GTest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# Eigen
find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
set(EIGEN3_LIBRARIES ${EIGEN3_LIBRARIES})
include_directories(${EIGEN3_INCLUDE_DIR})

# Add all the source files needed to build the executable
add_library(preimage SHARED
            src/bp/bp_solver.cpp
            src/bp/graph.cpp
            src/bp/node.cpp
            src/core/bit.cpp
            src/core/cnf.cpp
            src/core/config.cpp
            src/core/logic_gate.cpp
            src/core/solver.cpp
            src/core/sym_bit_vec.cpp
            src/core/sym_hash.cpp
            src/core/sym_representation.cpp
            src/core/utils.cpp
            src/cmsat/cmsat_solver.cpp
            src/hashing/sym_md5.cpp
            src/hashing/sym_ripemd160.cpp
            src/hashing/sym_sha256.cpp
            src/dag_solver/dag_solver.cpp)

target_link_libraries(preimage Boost::boost)
target_link_libraries(preimage cryptominisat5)

add_executable(main src/main.cpp)
target_link_libraries(main preimage)

if(BUILD_TESTS)
  enable_testing()

  add_executable(unit_tests
                 tests/test_md5.cpp
                 tests/test_ripemd160.cpp
                 tests/test_sha256.cpp
                 tests/test_sym_bit_vec.cpp
                 tests/test_utils.cpp)
  target_link_libraries(unit_tests gtest_main)
  target_link_libraries(unit_tests preimage)
  target_link_libraries(unit_tests cryptopp-shared)

  include(GoogleTest)
  gtest_discover_tests(unit_tests)
endif()
