cmake_minimum_required(VERSION 3.16)
project(blaze VERSION 0.0.1 LANGUAGES CXX
  DESCRIPTION "The ultra high-performance JSON Schema evaluator"
  HOMEPAGE_URL "https://github.com/sourcemeta/blaze")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(BLAZE_COMPILER "Build the Blaze compiler library" ON)
option(BLAZE_EVALUATOR "Build the Blaze evaluator library" ON)
option(BLAZE_OUTPUT "Build the Blaze output formats library" ON)
option(BLAZE_TEST "Build the Blaze test runner library" ON)
option(BLAZE_CONFIGURATION "Build the Blaze configuration file library" ON)
option(BLAZE_ALTERSCHEMA "Build the Blaze alterschema rule library" ON)
option(BLAZE_CODEGEN "Build the Blaze codegen library" ON)
option(BLAZE_DOCUMENTATION "Build the Blaze documentation generator library" ON)
option(BLAZE_TESTS "Build the Blaze tests" OFF)
option(BLAZE_BENCHMARK "Build the Blaze benchmarks" OFF)
option(BLAZE_CONTRIB "Build the Blaze contrib programs" OFF)
option(BLAZE_DOCS "Build the Blaze docs" OFF)
option(BLAZE_INSTALL "Install the Blaze library" ON)
option(BLAZE_ADDRESS_SANITIZER "Build Blaze with an address sanitizer" OFF)
option(BLAZE_UNDEFINED_SANITIZER "Build Blaze with an undefined behavior sanitizer" OFF)

if(BLAZE_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    COMPATIBILITY SameMajorVersion)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT sourcemeta_blaze_dev)
endif()

find_package(Core REQUIRED)

# Don't force downstream consumers on it
if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_enable_simd()
endif()

if(BLAZE_COMPILER)
  add_subdirectory(src/compiler)
endif()

if(BLAZE_EVALUATOR)
  add_subdirectory(src/evaluator)
endif()

if(BLAZE_OUTPUT)
  add_subdirectory(src/output)
endif()

if(BLAZE_TEST)
  add_subdirectory(src/test)
endif()

if(BLAZE_CONFIGURATION)
  add_subdirectory(src/configuration)
endif()

if(BLAZE_ALTERSCHEMA)
  add_subdirectory(src/alterschema)
endif()

if(BLAZE_CODEGEN)
  add_subdirectory(src/codegen)
endif()

if(BLAZE_DOCUMENTATION)
  add_subdirectory(src/documentation)
endif()

if(BLAZE_CONTRIB)
  add_subdirectory(contrib)
endif()

if(BLAZE_ADDRESS_SANITIZER)
  sourcemeta_sanitizer(TYPE address)
elseif(BLAZE_UNDEFINED_SANITIZER)
  sourcemeta_sanitizer(TYPE undefined)
endif()

if(BLAZE_DOCS)
  sourcemeta_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_target_clang_format(SOURCES
    contrib/*.cc
    src/*.h src/*.cc
    benchmark/*.h benchmark/*.cc
    test/*.h test/*.cc)

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    add_custom_target(blaze_format_trace_json
      COMMAND "${Python3_EXECUTABLE}"
        "${PROJECT_SOURCE_DIR}/contrib/format_trace_json.py"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_draft4.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_draft6.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_draft7.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_2019_09.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_2020_12.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_openapi_3_1.json"
        "${PROJECT_SOURCE_DIR}/test/evaluator/evaluator_openapi_3_2.json"
      COMMENT "Formatting trace JSON test files"
      VERBATIM)
  endif()
endif()

# Testing
if(BLAZE_TESTS)
  enable_testing()

  if(BLAZE_COMPILER)
    add_subdirectory(test/compiler)
  endif()

  if(BLAZE_EVALUATOR)
    add_subdirectory(test/evaluator)
  endif()

  if(BLAZE_OUTPUT)
    add_subdirectory(test/output)
  endif()

  if(BLAZE_TEST)
    add_subdirectory(test/test)
  endif()

  if(BLAZE_CONFIGURATION)
    add_subdirectory(test/configuration)
  endif()

  if(BLAZE_ALTERSCHEMA)
    add_subdirectory(test/alterschema)
  endif()

  if(BLAZE_CODEGEN)
    add_subdirectory(test/codegen)
  endif()

  if(BLAZE_DOCUMENTATION)
    add_subdirectory(test/documentation)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT BLAZE_ADDRESS_SANITIZER AND NOT BLAZE_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()

  if(BLAZE_BENCHMARK)
    add_subdirectory(benchmark)
  endif()
endif()
