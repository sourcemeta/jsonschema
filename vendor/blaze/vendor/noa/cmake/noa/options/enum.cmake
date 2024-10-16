function(noa_option_enum)
  cmake_parse_arguments(NOA_OPTION_ENUM "" "NAME;DEFAULT;DESCRIPTION" "CHOICES" ${ARGN})

  if(NOT NOA_OPTION_ENUM_NAME)
    message(FATAL_ERROR "You must pass the option name as NAME")
  endif()
  if(NOT NOA_OPTION_ENUM_DEFAULT)
    message(FATAL_ERROR "You must pass the option default value as DEFAULT")
  endif()
  if(NOT "${NOA_OPTION_ENUM_DEFAULT}" IN_LIST NOA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "Default value of ${NOA_OPTION_ENUM_NAME} must be one of these: ${NOA_OPTION_ENUM_CHOICES}")
  endif()
  if(NOT NOA_OPTION_ENUM_DESCRIPTION)
    message(FATAL_ERROR "You must pass the option description as DESCRIPTION")
  endif()
  if(NOT NOA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "You must pass the option enum choices as CHOICES")
  endif()

  # Declare the option
  set("${NOA_OPTION_ENUM_NAME}" "${NOA_OPTION_ENUM_DEFAULT}"
    CACHE STRING "${NOA_OPTION_ENUM_DESCRIPTION}")

  # Display a nice set of options in `cmake-gui`
  set_property(CACHE "${NOA_OPTION_ENUM_NAME}"
    PROPERTY STRINGS ${NOA_OPTION_ENUM_CHOICES})

  # Perform validation
  if(NOT "${${NOA_OPTION_ENUM_NAME}}" IN_LIST NOA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "Value of ${NOA_OPTION_ENUM_NAME} must be one of these: ${NOA_OPTION_ENUM_CHOICES}")
  endif()
endfunction()
