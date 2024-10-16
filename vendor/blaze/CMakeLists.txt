cmake_minimum_required(VERSION 3.16)
project(blaze VERSION 0.0.1 LANGUAGES CXX
  DESCRIPTION "The ultra high-performance JSON Schema evaluator"
  HOMEPAGE_URL "https://github.com/sourcemeta/blaze")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(vendor/noa/cmake/noa.cmake)

# Options
option(BLAZE_COMPILER "Build the Blaze compiler library" ON)
option(BLAZE_EVALUATOR "Build the Blaze evaluator library" ON)
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

if(BLAZE_COMPILER OR BLAZE_EVALUATOR)
  find_package(JSONToolkit REQUIRED)
endif()

if(BLAZE_COMPILER)
  add_subdirectory(src/compiler)
endif()

if(BLAZE_EVALUATOR)
  add_subdirectory(src/evaluator)
endif()

if(BLAZE_CONTRIB)
  add_subdirectory(contrib)
endif()

if(BLAZE_ADDRESS_SANITIZER)
  noa_sanitizer(TYPE address)
elseif(BLAZE_UNDEFINED_SANITIZER)
  noa_sanitizer(TYPE undefined)
endif()

if(BLAZE_DOCS)
  noa_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  noa_target_clang_format(SOURCES
    bindings/*.cc contrib/*.cc
    src/*.h src/*.cc
    benchmark/*.h benchmark/*.cc
    test/*.h test/*.cc)
  noa_target_clang_tidy(SOURCES
    src/*.h src/*.cc)
endif()

# Testing
if(BLAZE_TESTS)
  find_package(GoogleTest REQUIRED)
  enable_testing()

  if(BLAZE_COMPILER)
    add_subdirectory(test/compiler)
  endif()

  if(BLAZE_EVALUATOR)
    add_subdirectory(test/evaluator)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT BLAZE_ADDRESS_SANITIZER AND NOT BLAZE_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()

  if(BLAZE_BENCHMARK)
    find_package(GoogleBenchmark REQUIRED)
    add_subdirectory(benchmark)
  endif()
endif()
