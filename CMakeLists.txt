cmake_minimum_required(VERSION 3.16)
file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" JSONSCHEMA_VERSION)
string(STRIP "${JSONSCHEMA_VERSION}" JSONSCHEMA_VERSION)
project(jsonschema VERSION "${JSONSCHEMA_VERSION}" LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(JSONSCHEMA_TESTS "Build the JSON Schema CLI tests" OFF)
option(JSONSCHEMA_TESTS_CI "Build the JSON Schema CLI CI tests" OFF)
option(JSONSCHEMA_DEVELOPMENT "Build the JSON Schema CLI in development mode" OFF)
option(JSONSCHEMA_CONTINUOUS "Perform a continuous JSON Schema CLI release" ON)
option(JSONSCHEMA_PORTABLE "Build a portable JSON Schema CLI binary to increase platform support" OFF)
option(JSONSCHEMA_SYSTEM_HTTP "Use system HTTP dependencies instead of vendored ones" OFF)
option(JSONSCHEMA_USE_SYSTEM_OPENSSL "Use the system OpenSSL library" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(JSONSCHEMA_COMPILER_LLVM ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(JSONSCHEMA_COMPILER_GCC ON)
elseif(MSVC)
  set(JSONSCHEMA_COMPILER_MSVC ON)
endif()

find_package(Core REQUIRED)
if(NOT JSONSCHEMA_PORTABLE AND NOT MSVC)
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
if(JSONSCHEMA_SYSTEM_HTTP)
  find_package(cpr CONFIG REQUIRED)
  if(NOT TARGET CPR::cpr AND TARGET cpr::cpr)
    add_library(CPR::cpr INTERFACE IMPORTED)
    target_link_libraries(CPR::cpr INTERFACE cpr::cpr)
  endif()
else()
  find_package(ZLIB REQUIRED)
  if(JSONSCHEMA_USE_SYSTEM_OPENSSL)
    find_package(OpenSSL REQUIRED)
  elseif(NOT WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    find_package(MbedTLS REQUIRED)
  endif()
  find_package(Nghttp2 REQUIRED)
  if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    find_package(CAres REQUIRED)
  endif()
  find_package(PSL REQUIRED)
  find_package(CURL REQUIRED)
  find_package(CPR REQUIRED)
endif()
add_subdirectory(src)

if(JSONSCHEMA_DEVELOPMENT)
  sourcemeta_target_clang_format(SOURCES
    src/*.h src/*.cc)
  sourcemeta_target_shellcheck(SOURCES
    test/*.sh install *.sh completion/*)
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
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
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
    if(NOT GIT_BIN)
      message(STATUS "Could not determine current Git SHA: Git not available")
    elseif(NOT EXISTS "${GIT_DIR}")
      message(STATUS "Could not determine current Git SHA: Git directory does not exist (${GIT_DIR})")
    endif()

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
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    find_program(LDD_BIN NAMES ldd)
    if(LDD_BIN)
      execute_process(COMMAND "${LDD_BIN}" --version
        OUTPUT_VARIABLE LDD_OUTPUT
        ERROR_VARIABLE LDD_OUTPUT)
      if(LDD_OUTPUT MATCHES "musl")
        set(PLATFORM_SUFFIX "${PLATFORM_SUFFIX}-musl")
      endif()
    endif()
  endif()
  if(JSONSCHEMA_CONTINUOUS)
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PROJECT_GIT_SHA}-${PLATFORM_SUFFIX}")
  else()
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${PLATFORM_SUFFIX}")
  endif()
  set(CPACK_VERBATIM_VARIABLES YES)
  include(CPack)
endif()
