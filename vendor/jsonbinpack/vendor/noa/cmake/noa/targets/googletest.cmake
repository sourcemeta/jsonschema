function(noa_googletest)
  cmake_parse_arguments(NOA_GOOGLETEST ""
    "NAMESPACE;PROJECT;NAME;FOLDER;VARIANT" "SOURCES" ${ARGN})

  if(NOA_GOOGLETEST_VARIANT)
    set(TARGET_VARIANT "${NOA_GOOGLETEST_VARIANT}_unit")
  else()
    set(TARGET_VARIANT "unit")
  endif()

  noa_executable(
    NAMESPACE "${NOA_GOOGLETEST_NAMESPACE}"
    PROJECT "${NOA_GOOGLETEST_PROJECT}"
    NAME "${NOA_GOOGLETEST_NAME}"
    FOLDER "${NOA_GOOGLETEST_FOLDER}"
    VARIANT "${TARGET_VARIANT}"
    SOURCES "${NOA_GOOGLETEST_SOURCES}"
    OUTPUT TARGET_NAME)

  target_link_libraries("${TARGET_NAME}"
    PRIVATE GTest::gtest GTest::gmock GTest::gtest_main)
  add_test(NAME "${NOA_GOOGLETEST_PROJECT}.${NOA_GOOGLETEST_NAME}"
    COMMAND "${TARGET_NAME}" --gtest_brief=1)
endfunction()
