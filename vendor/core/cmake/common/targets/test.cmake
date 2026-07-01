function(sourcemeta_test)
  cmake_parse_arguments(SOURCEMETA_TEST ""
    "NAMESPACE;PROJECT;NAME;VARIANT" "SOURCES" ${ARGN})

  if(SOURCEMETA_TEST_VARIANT)
    set(TARGET_VARIANT "${SOURCEMETA_TEST_VARIANT}_unit")
  else()
    set(TARGET_VARIANT "unit")
  endif()

  sourcemeta_executable(
    NAMESPACE "${SOURCEMETA_TEST_NAMESPACE}"
    PROJECT "${SOURCEMETA_TEST_PROJECT}"
    NAME "${SOURCEMETA_TEST_NAME}"
    VARIANT "${TARGET_VARIANT}"
    SOURCES "${SOURCEMETA_TEST_SOURCES}"
    OUTPUT TARGET_NAME)

  target_link_libraries("${TARGET_NAME}"
    PRIVATE sourcemeta::core::test)
  # Provides a default entry point through static archive resolution unless the
  # suite defines its own `main`
  target_link_libraries("${TARGET_NAME}"
    PRIVATE sourcemeta::core::test_main)

  # Test executables are not shipped, so LTO buys nothing and significantly
  # slows the link step (GCC's LTRANS phase serializes per executable)
  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options("${TARGET_NAME}" PRIVATE -fno-lto)
    target_link_options("${TARGET_NAME}" PRIVATE -fno-lto)
  endif()

  add_test(NAME "${SOURCEMETA_TEST_PROJECT}.${SOURCEMETA_TEST_NAME}"
    COMMAND "${TARGET_NAME}")
endfunction()
