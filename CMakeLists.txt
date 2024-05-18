cmake_minimum_required(VERSION 3.22)
project(jsonschema VERSION 0.0.1 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(vendor/noa/cmake/noa.cmake)
include(cmake/CompilerOptions.cmake)

# Options
option(JSONSCHEMA_TESTS "Build the JSON Schema CLI tests" OFF)

find_package(JSONToolkit REQUIRED)
add_subdirectory(src)

noa_target_clang_format(SOURCES
  src/*.h src/*.cc)
noa_target_clang_tidy(SOURCES
  src/*.h src/*.cc)
noa_target_shellcheck(SOURCES
  test/*.sh)

# Testing
if(JSONSCHEMA_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# Packaging
set(CPACK_GENERATOR ZIP)
string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
set(PLATFORM_SUFFIX "${LOWER_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_SUFFIX}")
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
