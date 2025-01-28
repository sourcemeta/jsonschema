function(sourcemeta_target_shellcheck)
  cmake_parse_arguments(SOURCEMETA_TARGET_SHELLCHECK "REQUIRED" "" "SOURCES" ${ARGN})

  if(SOURCEMETA_TARGET_SHELLCHECK_REQUIRED)
    find_program(SHELLCHECK_BIN NAMES shellcheck REQUIRED)
  else()
    find_program(SHELLCHECK_BIN NAMES shellcheck)
  endif()

  # This covers the empty list too
  if(NOT SOURCEMETA_TARGET_SHELLCHECK_SOURCES)
    message(FATAL_ERROR "You must pass file globs to lint in the SOURCES option")
  endif()
  file(GLOB_RECURSE SOURCEMETA_TARGET_SHELLCHECK_FILES
    ${SOURCEMETA_TARGET_SHELLCHECK_SOURCES})

  if(SHELLCHECK_BIN)
    add_custom_target(shellcheck
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${SHELLCHECK_BIN}" ${SOURCEMETA_TARGET_SHELLCHECK_FILES}
      COMMENT "Analyzing sources using ShellCheck")
  else()
    add_custom_target(shellcheck
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate ShellCheck"
      COMMAND "${CMAKE_COMMAND}" -E false)
  endif()

  set_target_properties(shellcheck PROPERTIES FOLDER "Linting")
endfunction()
