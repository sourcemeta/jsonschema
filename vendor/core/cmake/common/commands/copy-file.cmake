function(sourcemeta_command_copy_file)
  cmake_parse_arguments(SOURCEMETA_COMMAND_COPY_FILE "" "FROM;TO" "" ${ARGN})

  if(NOT SOURCEMETA_COMMAND_COPY_FILE_FROM)
    message(FATAL_ERROR "You must pass the file to copy using the FROM option")
  endif()
  if(NOT SOURCEMETA_COMMAND_COPY_FILE_TO)
    message(FATAL_ERROR "You must pass the destination to copy to using the TO option")
  endif()

  add_custom_command(
    OUTPUT "${SOURCEMETA_COMMAND_COPY_FILE_TO}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${SOURCEMETA_COMMAND_COPY_FILE_FROM}" "${SOURCEMETA_COMMAND_COPY_FILE_TO}"
    MAIN_DEPENDENCY "${SOURCEMETA_COMMAND_COPY_FILE_FROM}"
    DEPENDS "${SOURCEMETA_COMMAND_COPY_FILE_FROM}"
    COMMENT "Copying ${SOURCEMETA_COMMAND_COPY_FILE_FROM} ot ${SOURCEMETA_COMMAND_COPY_FILE_TO}")
endfunction()
