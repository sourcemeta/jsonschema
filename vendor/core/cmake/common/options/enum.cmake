function(sourcemeta_option_enum)
  cmake_parse_arguments(SOURCEMETA_OPTION_ENUM "" "NAME;DEFAULT;DESCRIPTION" "CHOICES" ${ARGN})

  if(NOT SOURCEMETA_OPTION_ENUM_NAME)
    message(FATAL_ERROR "You must pass the option name as NAME")
  endif()
  if(NOT SOURCEMETA_OPTION_ENUM_DEFAULT)
    message(FATAL_ERROR "You must pass the option default value as DEFAULT")
  endif()
  if(NOT "${SOURCEMETA_OPTION_ENUM_DEFAULT}" IN_LIST SOURCEMETA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "Default value of ${SOURCEMETA_OPTION_ENUM_NAME} must be one of these: ${SOURCEMETA_OPTION_ENUM_CHOICES}")
  endif()
  if(NOT SOURCEMETA_OPTION_ENUM_DESCRIPTION)
    message(FATAL_ERROR "You must pass the option description as DESCRIPTION")
  endif()
  if(NOT SOURCEMETA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "You must pass the option enum choices as CHOICES")
  endif()

  # Declare the option
  set("${SOURCEMETA_OPTION_ENUM_NAME}" "${SOURCEMETA_OPTION_ENUM_DEFAULT}"
    CACHE STRING "${SOURCEMETA_OPTION_ENUM_DESCRIPTION}")

  # Display a nice set of options in `cmake-gui`
  set_property(CACHE "${SOURCEMETA_OPTION_ENUM_NAME}"
    PROPERTY STRINGS ${SOURCEMETA_OPTION_ENUM_CHOICES})

  # Perform validation
  if(NOT "${${SOURCEMETA_OPTION_ENUM_NAME}}" IN_LIST SOURCEMETA_OPTION_ENUM_CHOICES)
    message(FATAL_ERROR "Value of ${SOURCEMETA_OPTION_ENUM_NAME} must be one of these: ${SOURCEMETA_OPTION_ENUM_CHOICES}")
  endif()
endfunction()
