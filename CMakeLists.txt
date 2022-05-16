cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(CXX)

project(cryptosym)

set(CMAKE_BUILD_TYPE Release)
set(CRYPTOSYM_COMMAND_LINE_TOOL ON)
set(CRYPTOSYM_PYTHON_BINDINGS ON)
set(CRYPTOSYM_TESTS ON)

include(FetchContent)
include(ExternalProject)
include_directories(include)

# Automatically fetch/update git submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
  option(GIT_SUBMODULE "Check submodules during build" ON)
  if(GIT_SUBMODULE)
    message(STATUS "Submodule update")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

# Eigen (header-only library)
include_directories(extern/eigen)

# CryptoMiniSAT
set(MIT ON CACHE BOOL "" FORCE)
set(ONLY_SIMPLE ON CACHE BOOL "" FORCE)
set(STATS OFF CACHE BOOL "" FORCE)
set(NOZLIB ON CACHE BOOL "" FORCE)
set(NOM4RI ON CACHE BOOL "" FORCE)
set(NOSQLITE ON CACHE BOOL "" FORCE)
set(NOVALGRIND ON CACHE BOOL "" FORCE)
set(NOBREAKID ON CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PYTHON_INTERFACE OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/cryptominisat)
include_directories("${cryptominisat5_BINARY_DIR}/cmsat5-src")

if(CRYPTOSYM_TESTS)
  include_directories(tests/helpers/include)

  # Crypto++
  FetchContent_Declare(
    cryptopp
    GIT_REPOSITORY https://github.com/trevphil/cryptopp
    GIT_TAG 49fd7ffb03b9c6fa35b56e4e17904f6e124cf76f  # Release 8.6.0
  )
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)  # Don't build Crypto++ tests
  FetchContent_MakeAvailable(cryptopp)
  set(cryptopp_DIR "${cryptopp_BINARY_DIR}")
  find_package(cryptopp REQUIRED CONFIGS cryptopp-config-version.cmake)

  # Google Test
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# Add all the source files needed to build the library
add_library(cryptosymlib SHARED
            src/bp/bp_solver.cpp
            src/bp/graph.cpp
            src/bp/node.cpp
            src/core/bit_vec.cpp
            src/core/cnf.cpp
            src/core/config.cpp
            src/core/logic_gate.cpp
            src/core/solver.cpp
            src/core/sym_bit.cpp
            src/core/sym_bit_vec.cpp
            src/core/sym_hash.cpp
            src/core/sym_representation.cpp
            src/core/utils.cpp
            src/cmsat/cmsat_solver.cpp
            src/hashing/sym_md5.cpp
            src/hashing/sym_ripemd160.cpp
            src/hashing/sym_sha256.cpp
            src/dag_solver/dag_solver.cpp)

target_compile_options(cryptosymlib PRIVATE -Werror -Wall -Wextra -O3)
target_link_libraries(cryptosymlib cryptominisat5)

# Command-line tool
if(CRYPTOSYM_COMMAND_LINE_TOOL)
  add_executable(main src/main.cpp)
  target_link_libraries(main cryptosymlib)
endif()

# Python bindings via PyBind11
if(CRYPTOSYM_PYTHON_BINDINGS)
  add_subdirectory(extern/pybind11)
  pybind11_add_module(cryptosym src/bindings.cpp)
  target_link_libraries(cryptosym PUBLIC cryptosymlib)
endif()

# C++ tests
if(CRYPTOSYM_TESTS)
  enable_testing()

  add_executable(unit_tests
                 tests/helpers/src/eval_solver.cpp
                 tests/test_bit_vec.cpp
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
