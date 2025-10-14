cmake_minimum_required(VERSION 3.16)
project(jsonschema VERSION 11.11.0 LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(JSONSCHEMA_TESTS "Build the JSON Schema CLI tests" OFF)
option(JSONSCHEMA_TESTS_CI "Build the JSON Schema CLI CI tests" OFF)
option(JSONSCHEMA_DEVELOPMENT "Build the JSON Schema CLI in development mode" OFF)
option(JSONSCHEMA_CONTINUOUS "Perform a continuous JSON Schema CLI release" ON)
option(JSONSCHEMA_PORTABLE "Build a portable JSON Schema CLI binary to increase platform support" OFF)

find_package(Core REQUIRED)
if(NOT JSONSCHEMA_PORTABLE)
  message(STATUS "Building an optimised but less portable binary (JSONSCHEMA_PORTABLE)")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mtune=native")
endif()
# We can still enable SIMD for Apple targets, as Apple controls the hardware
if(NOT JSONSCHEMA_PORTABLE OR APPLE)
  sourcemeta_enable_simd()
endif()

find_package(Blaze REQUIRED)
find_package(JSONBinPack REQUIRED)
find_package(Hydra REQUIRED)
add_subdirectory(src)

if(JSONSCHEMA_DEVELOPMENT)
  sourcemeta_target_clang_format(SOURCES
    src/*.h src/*.cc)
  sourcemeta_target_shellcheck(SOURCES
    test/*.sh install *.sh)
endif()

# Testing
if(JSONSCHEMA_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(PROJECT_IS_TOP_LEVEL)
  # As a sanity check
  if(EXISTS "${PROJECT_SOURCE_DIR}/action.yml")
    file(READ "${PROJECT_SOURCE_DIR}/action.yml" ACTION_YML)
    string(FIND "${ACTION_YML}" "${PROJECT_VERSION}" ACTION_YML_HAS_VERSION)
    if(${ACTION_YML_HAS_VERSION} EQUAL -1)
      message(FATAL_ERROR
        "The GitHub Action definition must set the correct version: ${PROJECT_VERSION}")
    endif()
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
  if(LOWER_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(LOWER_SYSTEM_PROCESSOR "x86_64")
  endif()
  if(LOWER_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(LOWER_SYSTEM_PROCESSOR "arm64")
  endif()
  set(PLATFORM_SUFFIX "${LOWER_SYSTEM_NAME}-${LOWER_SYSTEM_PROCESSOR}")
  if(JSONSCHEMA_CONTINUOUS)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PROJECT_GIT_SHA}-${PLATFORM_SUFFIX}")
  else()
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_SUFFIX}")
  endif()
  set(CPACK_VERBATIM_VARIABLES YES)
  include(CPack)
endif()
