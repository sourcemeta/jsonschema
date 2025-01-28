function(sourcemeta_target_clang_format)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_FORMAT "REQUIRED" "" "SOURCES" ${ARGN})

  if(SOURCEMETA_TARGET_CLANG_FORMAT_REQUIRED)
    find_program(CLANG_FORMAT_BIN NAMES clang-format REQUIRED)
  else()
    find_program(CLANG_FORMAT_BIN NAMES clang-format)
  endif()

  # This covers the empty list too
  if(NOT SOURCEMETA_TARGET_CLANG_FORMAT_SOURCES)
    message(FATAL_ERROR "You must pass file globs to format in the SOURCES option")
  endif()
  file(GLOB_RECURSE SOURCEMETA_TARGET_CLANG_FORMAT_FILES
    ${SOURCEMETA_TARGET_CLANG_FORMAT_SOURCES})

  set(CLANG_FORMAT_CONFIG "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/clang-format.config")
  if(CLANG_FORMAT_BIN)
    add_custom_target(clang_format
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CLANG_FORMAT_BIN}" "--style=file:${CLANG_FORMAT_CONFIG}"
        -i ${SOURCEMETA_TARGET_CLANG_FORMAT_FILES}
      COMMENT "Formatting sources using ClangFormat")
    add_custom_target(clang_format_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CLANG_FORMAT_BIN}" "--style=file:${CLANG_FORMAT_CONFIG}"
        --dry-run -Werror
        -i ${SOURCEMETA_TARGET_CLANG_FORMAT_FILES}
      COMMENT "Checking for ClangFormat compliance")
  else()
    add_custom_target(clang_format
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate ClangFormat"
      COMMAND "${CMAKE_COMMAND}" -E false)
    add_custom_target(clang_format_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate ClangFormat"
      COMMAND "${CMAKE_COMMAND}" -E false)
  endif()

  set_target_properties(clang_format clang_format_test PROPERTIES FOLDER "Formatting")
endfunction()
