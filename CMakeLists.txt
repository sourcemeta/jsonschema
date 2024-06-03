cmake_minimum_required(VERSION 3.22)
project(jsonschema VERSION 0.1.2 LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(vendor/noa/cmake/noa.cmake)
include(cmake/CompilerOptions.cmake)

# Options
option(JSONSCHEMA_TESTS "Build the JSON Schema CLI tests" OFF)
option(JSONSCHEMA_TESTS_CI "Build the JSON Schema CLI CI tests" OFF)
option(JSONSCHEMA_CONTINUOUS "Perform a continuous JSON Schema CLI release" ON)

find_package(JSONToolkit REQUIRED)
find_package(Hydra REQUIRED)
add_subdirectory(src)

noa_target_clang_format(SOURCES
  src/*.h src/*.cc)
noa_target_clang_tidy(SOURCES
  src/*.h src/*.cc)
noa_target_shellcheck(SOURCES
  test/*.sh install)

# Testing
if(JSONSCHEMA_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# Packaging
find_program(GIT_BIN NAMES git)
if(GIT_BIN AND JSONSCHEMA_CONTINUOUS)
  execute_process(COMMAND "${GIT_BIN}" rev-parse --git-dir
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_DIR)
endif()
if(GIT_BIN AND EXISTS "${GIT_DIR}")
  execute_process(
    COMMAND "${GIT_BIN}" rev-parse --short=8 HEAD
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE PROJECT_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
else()
  set(PROJECT_GIT_SHA "unknown")
endif()

set(CPACK_GENERATOR ZIP)
string(TOLOWER ${CMAKE_SYSTEM_NAME} LOWER_SYSTEM_NAME)
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} LOWER_SYSTEM_PROCESSOR)
if(LOWER_SYSTEM_PROCESSOR STREQUAL "AMD64")
  set(LOWER_SYSTEM_PROCESSOR "x86_64")
endif()
set(PLATFORM_SUFFIX "${LOWER_SYSTEM_NAME}-${LOWER_SYSTEM_PROCESSOR}")
if(JSONSCHEMA_CONTINUOUS)
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PROJECT_GIT_SHA}-${PLATFORM_SUFFIX}")
else()
  set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_SUFFIX}")
endif()
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
