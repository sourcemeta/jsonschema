cmake_minimum_required(VERSION 3.16)
project(noa VERSION 0.0.0 LANGUAGES CXX
  DESCRIPTION "A set of re-usable and opinionated utilities for Sourcemeta projects")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(noa)

# Options
option(NOA_HASH "Build the Noa Hash library" ON)
option(NOA_FLAT_MAP "Build the Noa Flat Map library" ON)
option(NOA_REGEX "Build the Noa Regex library" ON)
option(NOA_GOOGLETEST "Build the Google Test library" ON)
option(NOA_GOOGLEBENCHMARK "Build the Google Benchmark library" ON)
option(NOA_TESTS "Build the Noa tests" OFF)
option(NOA_BENCHMARK "Build the Noa benchmarks" OFF)
option(NOA_DOCS "Build the Noa docs" OFF)
option(NOA_INSTALL "Install the Noa library" ON)
option(NOA_ADDRESS_SANITIZER "Build Noa with an address sanitizer" OFF)
option(NOA_UNDEFINED_SANITIZER "Build Noa with an undefined behavior sanitizer" OFF)

if(NOA_INSTALL)
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
    COMPONENT sourcemeta_noa_dev)
endif()

if(NOA_HASH)
  add_subdirectory(src/hash)
endif()

if(NOA_FLAT_MAP)
  add_subdirectory(src/flat_map)
endif()

if(NOA_REGEX)
  find_package(BoostRegex REQUIRED)
  add_subdirectory(src/regex)
endif()

if(NOA_GOOGLETEST)
  find_package(GoogleTest REQUIRED)
endif()

if(NOA_ADDRESS_SANITIZER)
  noa_sanitizer(TYPE address)
elseif(NOA_UNDEFINED_SANITIZER)
  noa_sanitizer(TYPE undefined)
endif()

if(NOA_DOCS)
  noa_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  noa_target_clang_format(SOURCES
    benchmark/*.h benchmark/*.cc
    src/*.h src/*.cc
    test/*.h test/*.cc)
  noa_target_clang_tidy(SOURCES
    src/*.h src/*.cc)
endif()

if(NOA_TESTS AND NOA_GOOGLETEST)
  enable_testing()

  if(NOA_HASH)
    add_subdirectory(test/hash)
  endif()

  if(NOA_FLAT_MAP)
    add_subdirectory(test/flat_map)
  endif()

  if(NOA_REGEX)
    add_subdirectory(test/regex)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT NOA_ADDRESS_SANITIZER AND NOT NOA_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()
endif()

if(NOA_GOOGLEBENCHMARK)
  find_package(GoogleBenchmark REQUIRED)
endif()

if(NOA_BENCHMARK AND NOA_GOOGLEBENCHMARK)
  add_subdirectory(benchmark)
endif()
