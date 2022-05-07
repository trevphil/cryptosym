cmake_minimum_required(VERSION 3.12)

project(cryptosym)

set(BUILD_TESTS OFF)

add_definitions(-std=c++17 -Wshadow -Wunused -Wall -Werror -O3)
# add_compile_options(-g)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Boost
find_package(Boost REQUIRED)

# CryptoMiniSAT
find_package(cryptominisat5 REQUIRED)

# Eigen
find_package(Eigen3 REQUIRED)
set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
set(EIGEN3_LIBRARIES ${EIGEN3_LIBRARIES})
include_directories(${EIGEN3_INCLUDE_DIR})

if(BUILD_TESTS)
  include_directories(tests/helpers/include)

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

# Add all the source files needed to build the executable
add_library(cryptosymlib SHARED
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

target_link_libraries(cryptosymlib Boost::boost)
target_link_libraries(cryptosymlib cryptominisat5)

add_executable(main src/main.cpp)
target_link_libraries(main cryptosymlib)

if(BUILD_TESTS)
  enable_testing()

  add_executable(unit_tests
                 tests/helpers/src/eval_solver.cpp
                 tests/test_bp_solver.cpp
                 tests/test_cmsat_solver.cpp
                 tests/test_cnf.cpp
                 tests/test_dag_solver.cpp
                 tests/test_logic_gate.cpp
                 tests/test_md5.cpp
                 tests/test_ripemd160.cpp
                 tests/test_sha256.cpp
                 tests/test_sym_bit_vec.cpp
                 tests/test_sym_representation.cpp
                 tests/test_utils.cpp)

  target_link_libraries(unit_tests gtest_main)
  target_link_libraries(unit_tests cryptosymlib)
  target_link_libraries(unit_tests cryptopp-shared)

  include(GoogleTest)
  gtest_discover_tests(unit_tests)
endif()

# PyBind11
add_subdirectory(extern/pybind11)
pybind11_add_module(cryptosym src/bindings.cpp)
target_link_libraries(cryptosym PUBLIC cryptosymlib)

# EXAMPLE_VERSION_INFO is defined by setup.py
# target_compile_definitions(cmake_example
#                            PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
