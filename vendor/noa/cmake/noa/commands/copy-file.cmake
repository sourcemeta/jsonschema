function(noa_command_copy_file)
  cmake_parse_arguments(NOA_COMMAND_COPY_FILE "" "FROM;TO" "" ${ARGN})

  if(NOT NOA_COMMAND_COPY_FILE_FROM)
    message(FATAL_ERROR "You must pass the file to copy using the FROM option")
  endif()
  if(NOT NOA_COMMAND_COPY_FILE_TO)
    message(FATAL_ERROR "You must pass the destination to copy to using the TO option")
  endif()

  add_custom_command(
    OUTPUT "${NOA_COMMAND_COPY_FILE_TO}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${NOA_COMMAND_COPY_FILE_FROM}" "${NOA_COMMAND_COPY_FILE_TO}"
    MAIN_DEPENDENCY "${NOA_COMMAND_COPY_FILE_FROM}"
    DEPENDS "${NOA_COMMAND_COPY_FILE_FROM}"
    COMMENT "Copying ${NOA_COMMAND_COPY_FILE_FROM} ot ${NOA_COMMAND_COPY_FILE_TO}")
endfunction()
