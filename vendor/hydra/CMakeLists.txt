cmake_minimum_required(VERSION 3.16)
project(hydra VERSION 0.0.1 LANGUAGES C CXX
  DESCRIPTION "A convenience networking library for modern C++")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(HYDRA_HTTP "Build the Hydra HTTP library" ON)
option(HYDRA_TESTS "Build the Hydra tests" OFF)
option(HYDRA_DOCS "Build the Hydra docs" OFF)
option(HYDRA_INSTALL "Install the Hydra library" ON)
option(HYDRA_ADDRESS_SANITIZER "Build Hydra with an address sanitizer" OFF)
option(HYDRA_UNDEFINED_SANITIZER "Build Hydra with an undefined behavior sanitizer" OFF)

find_package(Core REQUIRED)

if(HYDRA_INSTALL)
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
    COMPONENT sourcemeta_hydra_dev)
endif()

if(HYDRA_HTTP)
  find_package(CURL REQUIRED)
  add_subdirectory(src/http)
endif()

if(HYDRA_ADDRESS_SANITIZER)
  sourcemeta_sanitizer(TYPE address)
elseif(HYDRA_UNDEFINED_SANITIZER)
  sourcemeta_sanitizer(TYPE undefined)
endif()

if(HYDRA_DOCS)
  sourcemeta_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_target_clang_format(SOURCES
    src/*.cc src/*.h test/*.cc test/*.h)
  sourcemeta_target_clang_tidy(SOURCES src/*.cc)
endif()

# Testing
if(HYDRA_TESTS)
  enable_testing()

  if(HYDRA_HTTP)
    add_subdirectory(test/unit/http)
    add_subdirectory(test/e2e/http)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT HYDRA_ADDRESS_SANITIZER AND NOT HYDRA_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()
endif()
