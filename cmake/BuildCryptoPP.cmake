# Include usage of external projects
include(FetchContent)

# Define the release version we want to use for Crypto++
set(CRYPTOPP_GIT_TAG CRYPTOPP_8_6_0)

set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CRYPTOPP_DATA_DIR "" CACHE STRING "" FORCE)
set(cryptopp_DISPLAY_CMAKE_SUPPORT_WARNING OFF CACHE BOOL "" FORCE)

# Get the cryptopp CMakeLists.txt file for cryptopp package
set(CRYPTOPP_CMAKE "cryptopp-cmake")
FetchContent_Declare(
  ${CRYPTOPP_CMAKE}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/cryptopp-cmake
)

FetchContent_GetProperties(${CRYPTOPP_CMAKE})
if(NOT ${CRYPTOPP_CMAKE}_POPULATED)
  FetchContent_Populate(${CRYPTOPP_CMAKE})
endif()

# Get the cryptopp package
set(CRYPTOPP "cryptopp")
FetchContent_Declare(
  ${CRYPTOPP}
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/cryptopp
)

FetchContent_GetProperties(${CRYPTOPP})
if(NOT ${CRYPTOPP}_POPULATED)
  FetchContent_Populate(${CRYPTOPP})
endif()

file(COPY ${${CRYPTOPP_CMAKE}_SOURCE_DIR}/CMakeLists.txt DESTINATION ${${CRYPTOPP}_SOURCE_DIR})
add_subdirectory(${${CRYPTOPP}_SOURCE_DIR} ${${CRYPTOPP}_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/extern)
