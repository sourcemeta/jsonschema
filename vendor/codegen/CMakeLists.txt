cmake_minimum_required(VERSION 3.16)
project(codegen VERSION 0.0.0 LANGUAGES CXX
  DESCRIPTION "The JSON Schema code generator"
  HOMEPAGE_URL "https://github.com/sourcemeta/codegen")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(CODEGEN_IR "Build the Codegen IR library" ON)
option(CODEGEN_GENERATOR "Build the Codegen Generator library" ON)
option(CODEGEN_TESTS "Build the Codegen tests" OFF)
option(CODEGEN_CONTRIB "Build the Codegen contrib programs" OFF)
option(CODEGEN_DOCS "Build the Codegen docs" OFF)
option(CODEGEN_INSTALL "Install the Codegen library" ON)
option(CODEGEN_ADDRESS_SANITIZER "Build Codegen with an address sanitizer" OFF)
option(CODEGEN_UNDEFINED_SANITIZER "Build Codegen with an undefined behavior sanitizer" OFF)

if(CODEGEN_INSTALL)
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
    COMPONENT sourcemeta_codegen_dev)
endif()

find_package(Core REQUIRED)

# Don't force downstream consumers on it
if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_enable_simd()
endif()

if(CODEGEN_IR)
  add_subdirectory(src/ir)
endif()

if(CODEGEN_GENERATOR)
  add_subdirectory(src/generator)
endif()

if(CODEGEN_CONTRIB)
  add_subdirectory(contrib)
endif()

if(CODEGEN_ADDRESS_SANITIZER)
  sourcemeta_sanitizer(TYPE address)
elseif(CODEGEN_UNDEFINED_SANITIZER)
  sourcemeta_sanitizer(TYPE undefined)
endif()

if(CODEGEN_DOCS)
  sourcemeta_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_target_clang_format(SOURCES
    contrib/*.cc
    src/*.h src/*.cc
    test/*.h test/*.cc)
endif()

# Testing
if(CODEGEN_TESTS)
  enable_testing()

  if(CODEGEN_IR)
    add_subdirectory(test/ir)
  endif()

  if(CODEGEN_GENERATOR)
    add_subdirectory(test/e2e/typescript)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT CODEGEN_ADDRESS_SANITIZER AND NOT CODEGEN_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()

  add_custom_target(jsonschema_metaschema
    COMMAND "${PROJECT_SOURCE_DIR}/node_modules/.bin/jsonschema"
      metaschema ${E2E_SCHEMAS}
    VERBATIM)
endif()
