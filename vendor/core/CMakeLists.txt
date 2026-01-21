cmake_minimum_required(VERSION 3.16)
project(core VERSION 0.0.0 LANGUAGES C CXX ASM_MASM DESCRIPTION "Sourcemeta Core")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Options
option(SOURCEMETA_CORE_LANG_IO "Build the Sourcemeta Core language I/O library" ON)
option(SOURCEMETA_CORE_LANG_PROCESS "Build the Sourcemeta Core language Process library" ON)
option(SOURCEMETA_CORE_LANG_PARALLEL "Build the Sourcemeta Core language parallel library" ON)
option(SOURCEMETA_CORE_LANG_NUMERIC "Build the Sourcemeta Core language numeric library" ON)
option(SOURCEMETA_CORE_PUNYCODE "Build the Sourcemeta Core Punycode library" ON)
option(SOURCEMETA_CORE_TIME "Build the Sourcemeta Core time library" ON)
option(SOURCEMETA_CORE_UUID "Build the Sourcemeta Core UUID library" ON)
option(SOURCEMETA_CORE_MD5 "Build the Sourcemeta Core MD5 library" ON)
option(SOURCEMETA_CORE_REGEX "Build the Sourcemeta Core Regex library" ON)
option(SOURCEMETA_CORE_URI "Build the Sourcemeta Core URI library" ON)
option(SOURCEMETA_CORE_URITEMPLATE "Build the Sourcemeta Core URI Template library" ON)
option(SOURCEMETA_CORE_JSON "Build the Sourcemeta Core JSON library" ON)
option(SOURCEMETA_CORE_JSONSCHEMA "Build the Sourcemeta Core JSON Schema library" ON)
option(SOURCEMETA_CORE_JSONPOINTER "Build the Sourcemeta Core JSON Pointer library" ON)
option(SOURCEMETA_CORE_JSONL "Build the Sourcemeta Core JSONL library" ON)
option(SOURCEMETA_CORE_YAML "Build the Sourcemeta Core YAML library" ON)
option(SOURCEMETA_CORE_HTML "Build the Sourcemeta Core HTML library" ON)
option(SOURCEMETA_CORE_EXTENSION_ALTERSCHEMA "Build the Sourcemeta Core AlterSchema library" ON)
option(SOURCEMETA_CORE_EXTENSION_EDITORSCHEMA "Build the Sourcemeta Core EditorSchema library" ON)
option(SOURCEMETA_CORE_EXTENSION_SCHEMACONFIG "Build the Sourcemeta Core SchemaConfig library" ON)
option(SOURCEMETA_CORE_EXTENSION_OPTIONS "Build the Sourcemeta Core Options library" ON)
option(SOURCEMETA_CORE_EXTENSION_BUILD "Build the Sourcemeta Core Build library" ON)
option(SOURCEMETA_CORE_TESTS "Build the Sourcemeta Core tests" OFF)
option(SOURCEMETA_CORE_BENCHMARK "Build the Sourcemeta Core benchmarks" OFF)
option(SOURCEMETA_CORE_DOCS "Build the Sourcemeta Core docs" OFF)
option(SOURCEMETA_CORE_INSTALL "Install the Sourcemeta Core library" ON)
option(SOURCEMETA_CORE_ADDRESS_SANITIZER "Build Sourcemeta Core with an address sanitizer" OFF)
option(SOURCEMETA_CORE_UNDEFINED_SANITIZER "Build Sourcemeta Core with an undefined behavior sanitizer" OFF)
option(SOURCEMETA_CORE_CONTRIB_GOOGLETEST "Build the GoogleTest library for downstream consumers" OFF)
option(SOURCEMETA_CORE_CONTRIB_GOOGLEBENCHMARK "Build the GoogleBenchmark library for downstream consumers" OFF)

include(Sourcemeta)

# Don't force downstream consumers on this
if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_enable_simd()
endif()

# TODO: Turn this into a re-usable utility CMake function
if(SOURCEMETA_CORE_INSTALL)
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
    COMPONENT sourcemeta_${PROJECT_NAME}_dev)
endif()

if(SOURCEMETA_CORE_LANG_IO)
  add_subdirectory(src/lang/io)
endif()

if(SOURCEMETA_CORE_LANG_PROCESS)
  add_subdirectory(src/lang/process)
endif()

if(SOURCEMETA_CORE_LANG_PARALLEL)
  find_package(Threads REQUIRED)
  add_subdirectory(src/lang/parallel)
endif()

if(SOURCEMETA_CORE_LANG_NUMERIC)
  find_package(mpdecimal REQUIRED)
  add_subdirectory(src/lang/numeric)
endif()

if(SOURCEMETA_CORE_PUNYCODE)
  add_subdirectory(src/core/punycode)
endif()

if(SOURCEMETA_CORE_TIME)
  add_subdirectory(src/core/time)
endif()

if(SOURCEMETA_CORE_UUID)
  add_subdirectory(src/core/uuid)
endif()

if(SOURCEMETA_CORE_MD5)
  add_subdirectory(src/core/md5)
endif()

if(SOURCEMETA_CORE_REGEX)
  find_package(PCRE2 REQUIRED)
  add_subdirectory(src/core/regex)
endif()

if(SOURCEMETA_CORE_URI)
  add_subdirectory(src/core/uri)
endif()

if(SOURCEMETA_CORE_URITEMPLATE)
  add_subdirectory(src/core/uritemplate)
endif()

if(SOURCEMETA_CORE_JSON)
  add_subdirectory(src/core/json)
endif()

if(SOURCEMETA_CORE_JSONPOINTER)
  add_subdirectory(src/core/jsonpointer)
endif()

if(SOURCEMETA_CORE_JSONSCHEMA)
  add_subdirectory(src/core/jsonschema)
endif()

if(SOURCEMETA_CORE_JSONL)
  add_subdirectory(src/core/jsonl)
endif()

if(SOURCEMETA_CORE_YAML)
  find_package(yaml REQUIRED)
  add_subdirectory(src/core/yaml)
endif()

if(SOURCEMETA_CORE_HTML)
  add_subdirectory(src/core/html)
endif()

if(SOURCEMETA_CORE_EXTENSION_ALTERSCHEMA)
  add_subdirectory(src/extension/alterschema)
endif()

if(SOURCEMETA_CORE_EXTENSION_EDITORSCHEMA)
  add_subdirectory(src/extension/editorschema)
endif()

if(SOURCEMETA_CORE_EXTENSION_SCHEMACONFIG)
  add_subdirectory(src/extension/schemaconfig)
endif()

if(SOURCEMETA_CORE_EXTENSION_OPTIONS)
  add_subdirectory(src/extension/options)
endif()

if(SOURCEMETA_CORE_EXTENSION_BUILD)
  add_subdirectory(src/extension/build)
endif()

if(SOURCEMETA_CORE_ADDRESS_SANITIZER)
  sourcemeta_sanitizer(TYPE address)
elseif(SOURCEMETA_CORE_UNDEFINED_SANITIZER)
  sourcemeta_sanitizer(TYPE undefined)
endif()

if(SOURCEMETA_CORE_DOCS)
  sourcemeta_target_doxygen(CONFIG "${PROJECT_SOURCE_DIR}/doxygen/Doxyfile.in"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/website")
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_target_clang_format(SOURCES
    src/*.h src/*.cc
    benchmark/*.h benchmark/*.cc
    test/*.h test/*.cc)
endif()

# Testing

if(SOURCEMETA_CORE_CONTRIB_GOOGLETEST OR SOURCEMETA_CORE_TESTS)
  find_package(GoogleTest REQUIRED)
endif()

if(SOURCEMETA_CORE_CONTRIB_GOOGLEBENCHMARK OR SOURCEMETA_CORE_BENCHMARK)
  find_package(GoogleBenchmark REQUIRED)
endif()

if(SOURCEMETA_CORE_TESTS)
  enable_testing()

  if(SOURCEMETA_CORE_LANG_IO)
    add_subdirectory(test/io)
  endif()

  if(SOURCEMETA_CORE_LANG_PROCESS)
    add_subdirectory(test/process)
  endif()

  if(SOURCEMETA_CORE_LANG_PARALLEL)
    add_subdirectory(test/parallel)
  endif()

  if(SOURCEMETA_CORE_LANG_NUMERIC)
    add_subdirectory(test/numeric)
  endif()

  if(SOURCEMETA_CORE_PUNYCODE)
    add_subdirectory(test/punycode)
  endif()

  if(SOURCEMETA_CORE_TIME)
    add_subdirectory(test/time)
  endif()

  if(SOURCEMETA_CORE_UUID)
    add_subdirectory(test/uuid)
  endif()

  if(SOURCEMETA_CORE_MD5)
    add_subdirectory(test/md5)
  endif()

  if(SOURCEMETA_CORE_REGEX)
    add_subdirectory(test/regex)
  endif()

  if(SOURCEMETA_CORE_URI)
    add_subdirectory(test/uri)
  endif()

  if(SOURCEMETA_CORE_URITEMPLATE)
    add_subdirectory(test/uritemplate)
  endif()

  if(SOURCEMETA_CORE_JSON)
    add_subdirectory(test/json)
  endif()

  if(SOURCEMETA_CORE_JSONPOINTER)
    add_subdirectory(test/jsonpointer)
  endif()

  if(SOURCEMETA_CORE_JSONSCHEMA)
    add_subdirectory(test/jsonschema)
  endif()

  if(SOURCEMETA_CORE_JSONL)
    add_subdirectory(test/jsonl)
  endif()

  if(SOURCEMETA_CORE_YAML)
    add_subdirectory(test/yaml)
  endif()

  if(SOURCEMETA_CORE_HTML)
    add_subdirectory(test/html)
  endif()

  if(SOURCEMETA_CORE_EXTENSION_ALTERSCHEMA)
    add_subdirectory(test/alterschema)
  endif()

  if(SOURCEMETA_CORE_EXTENSION_EDITORSCHEMA)
    add_subdirectory(test/editorschema)
  endif()

  if(SOURCEMETA_CORE_EXTENSION_SCHEMACONFIG)
    add_subdirectory(test/schemaconfig)
  endif()

  if(SOURCEMETA_CORE_EXTENSION_OPTIONS)
    add_subdirectory(test/options)
  endif()

  if(SOURCEMETA_CORE_EXTENSION_BUILD)
    add_subdirectory(test/build)
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    # Otherwise we need the child project to link
    # against the sanitizers too.
    if(NOT SOURCEMETA_CORE_ADDRESS_SANITIZER AND NOT SOURCEMETA_CORE_UNDEFINED_SANITIZER)
      add_subdirectory(test/packaging)
    endif()
  endif()
endif()

if(SOURCEMETA_CORE_BENCHMARK)
  add_subdirectory(benchmark)
endif()
