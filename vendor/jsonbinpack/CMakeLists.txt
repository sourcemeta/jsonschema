cmake_minimum_required(VERSION 3.16)
project(jsonbinpack VERSION 0.0.1 LANGUAGES CXX
  DESCRIPTION "\
A space-efficient open-source binary JSON serialization \
format based on JSON Schema with \
both schema-driven and schema-less support."
  HOMEPAGE_URL "https://jsonbinpack.sourcemeta.com")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(JSONBINPACK_NUMERIC "Build the JSON BinPack numeric library" ON)
option(JSONBINPACK_RUNTIME "Build the JSON BinPack runtime" ON)
option(JSONBINPACK_COMPILER "Build the JSON BinPack compiler" ON)
option(JSONBINPACK_TESTS "Build the JSON BinPack tests" OFF)
option(JSONBINPACK_INSTALL "Install the JSON BinPack library" ON)
option(JSONBINPACK_DOCS "Build the JSON BinPack documentation" OFF)
option(JSONBINPACK_ADDRESS_SANITIZER "Build JSON BinPack with an address sanitizer" OFF)
option(JSONBINPACK_UNDEFINED_SANITIZER "Build JSON BinPack with an undefined behavior sanitizer" OFF)

find_package(Core REQUIRED)

if(JSONBINPACK_INSTALL)
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
    COMPONENT sourcemeta_jsonbinpack_dev)
endif()

# Dependencies
find_package(Core REQUIRED)

# Numeric
if(JSONBINPACK_NUMERIC)
  add_subdirectory(src/numeric)
endif()

# Runtime
if(JSONBINPACK_RUNTIME)
  add_subdirectory(src/runtime)
endif()

# Compiler
if(JSONBINPACK_COMPILER)
  add_subdirectory(src/compiler)
endif()

if(JSONBINPACK_ADDRESS_SANITIZER)
  sourcemeta_sanitizer(TYPE address)
elseif(JSONBINPACK_UNDEFINED_SANITIZER)
  sourcemeta_sanitizer(TYPE undefined)
endif()

if(JSONBINPACK_DOCS)
  sourcemeta_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/www")
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_target_clang_format(SOURCES
    src/*.h src/*.cc
    test/*.h test/*.cc)
  sourcemeta_target_clang_tidy(SOURCES src/*.cc)
endif()

# Testing
if(JSONBINPACK_TESTS)
  enable_testing()

  if(JSONBINPACK_NUMERIC)
    add_subdirectory(test/numeric)
  endif()

  if(JSONBINPACK_RUNTIME)
    add_subdirectory(test/runtime)
  endif()

  if(JSONBINPACK_COMPILER)
    add_subdirectory(test/compiler)
  endif()

  add_subdirectory(test/e2e)

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT JSONBINPACK_ADDRESS_SANITIZER AND NOT JSONBINPACK_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()
endif()
