function(sourcemeta_googletest)
  cmake_parse_arguments(SOURCEMETA_GOOGLETEST ""
    "NAMESPACE;PROJECT;NAME;VARIANT" "SOURCES" ${ARGN})

  if(SOURCEMETA_GOOGLETEST_VARIANT)
    set(TARGET_VARIANT "${SOURCEMETA_GOOGLETEST_VARIANT}_unit")
  else()
    set(TARGET_VARIANT "unit")
  endif()

  sourcemeta_executable(
    NAMESPACE "${SOURCEMETA_GOOGLETEST_NAMESPACE}"
    PROJECT "${SOURCEMETA_GOOGLETEST_PROJECT}"
    NAME "${SOURCEMETA_GOOGLETEST_NAME}"
    VARIANT "${TARGET_VARIANT}"
    SOURCES "${SOURCEMETA_GOOGLETEST_SOURCES}"
    OUTPUT TARGET_NAME)

  target_link_libraries("${TARGET_NAME}"
    PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)

  # Test executables are not shipped, so LTO buys nothing and significantly
  # slows the link step (GCC's LTRANS phase serializes per executable)
  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options("${TARGET_NAME}" PRIVATE -fno-lto)
    target_link_options("${TARGET_NAME}" PRIVATE -fno-lto)
  endif()

  add_test(NAME "${SOURCEMETA_GOOGLETEST_PROJECT}.${SOURCEMETA_GOOGLETEST_NAME}"
    COMMAND "${TARGET_NAME}" --gtest_brief=1)
endfunction()
