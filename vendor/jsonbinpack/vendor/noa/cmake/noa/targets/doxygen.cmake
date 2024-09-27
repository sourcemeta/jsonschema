function(noa_target_doxygen)
  cmake_parse_arguments(NOA_TARGET_DOXYGEN "" "CONFIG;OUTPUT" "" ${ARGN})

  if(NOT NOA_TARGET_DOXYGEN_CONFIG)
    message(FATAL_ERROR "You must pass an input config file using the CONFIG option")
  endif()
  if(NOT NOA_TARGET_DOXYGEN_OUTPUT)
    message(FATAL_ERROR "You must pass an output directory using the OUTPUT option")
  endif()

  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_IN "${NOA_TARGET_DOXYGEN_CONFIG}")
    set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
    configure_file("${DOXYGEN_IN}" "${DOXYGEN_OUT}" @ONLY)
    add_custom_target(doxygen
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${NOA_TARGET_DOXYGEN_OUTPUT}"
      COMMAND "${DOXYGEN_EXECUTABLE}" "${DOXYGEN_OUT}")
  else()
    add_custom_target(doxygen VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate Doxygen"
      COMMAND "${CMAKE_COMMAND}" -E false)
  endif()
endfunction()
