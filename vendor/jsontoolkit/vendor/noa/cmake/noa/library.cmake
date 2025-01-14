function(noa_library)
  cmake_parse_arguments(NOA_LIBRARY ""
    "NAMESPACE;PROJECT;NAME;FOLDER;VARIANT" "PRIVATE_HEADERS;SOURCES" ${ARGN})

  if(NOT NOA_LIBRARY_PROJECT)
    message(FATAL_ERROR "You must pass the project name using the PROJECT option")
  endif()
  if(NOT NOA_LIBRARY_NAME)
    message(FATAL_ERROR "You must pass the library name using the NAME option")
  endif()
  if(NOT NOA_LIBRARY_FOLDER)
    message(FATAL_ERROR "You must pass the folder name using the FOLDER option")
  endif()

  if(NOA_LIBRARY_NAMESPACE)
    set(INCLUDE_PREFIX "include/${NOA_LIBRARY_NAMESPACE}/${NOA_LIBRARY_PROJECT}")
  else()
    set(INCLUDE_PREFIX "include/${NOA_LIBRARY_PROJECT}")
  endif()

  set(EXPORT_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${INCLUDE_PREFIX}/${NOA_LIBRARY_NAME}_export.h")
  if(NOT NOA_LIBRARY_VARIANT)
    set(PUBLIC_HEADER "${INCLUDE_PREFIX}/${NOA_LIBRARY_NAME}.h")
  else()
    set(PUBLIC_HEADER "../${INCLUDE_PREFIX}/${NOA_LIBRARY_NAME}.h")
  endif()

  if(NOA_LIBRARY_SOURCES)
    set(ABSOLUTE_PRIVATE_HEADERS "${EXPORT_HEADER_PATH}")
  else()
    set(ABSOLUTE_PRIVATE_HEADERS)
  endif()

  foreach(private_header IN LISTS NOA_LIBRARY_PRIVATE_HEADERS)
    if(NOA_LIBRARY_VARIANT)
      list(APPEND ABSOLUTE_PRIVATE_HEADERS "../${INCLUDE_PREFIX}/${NOA_LIBRARY_NAME}_${private_header}")
    else()
      list(APPEND ABSOLUTE_PRIVATE_HEADERS "${INCLUDE_PREFIX}/${NOA_LIBRARY_NAME}_${private_header}")
    endif()
  endforeach()

  if(NOA_LIBRARY_NAMESPACE)
    set(TARGET_NAME "${NOA_LIBRARY_NAMESPACE}_${NOA_LIBRARY_PROJECT}_${NOA_LIBRARY_NAME}")
    set(ALIAS_NAME "${NOA_LIBRARY_NAMESPACE}::${NOA_LIBRARY_PROJECT}::${NOA_LIBRARY_NAME}")
  else()
    set(TARGET_NAME "${NOA_LIBRARY_PROJECT}_${NOA_LIBRARY_NAME}")
    set(ALIAS_NAME "${NOA_LIBRARY_PROJECT}::${NOA_LIBRARY_NAME}")
  endif()

  if(NOA_LIBRARY_VARIANT)
    set(TARGET_NAME "${TARGET_NAME}_${NOA_LIBRARY_VARIANT}")
    set(ALIAS_NAME "${ALIAS_NAME}::${NOA_LIBRARY_VARIANT}")
  endif()

  if(NOA_LIBRARY_SOURCES)
    add_library(${TARGET_NAME}
      ${PUBLIC_HEADER} ${ABSOLUTE_PRIVATE_HEADERS} ${NOA_LIBRARY_SOURCES})
    noa_add_default_options(PRIVATE ${TARGET_NAME})
  else()
    add_library(${TARGET_NAME} INTERFACE
      ${PUBLIC_HEADER} ${ABSOLUTE_PRIVATE_HEADERS})
    noa_add_default_options(INTERFACE ${TARGET_NAME})
  endif()

  add_library(${ALIAS_NAME} ALIAS ${TARGET_NAME})

  if(NOT NOA_LIBRARY_VARIANT)
    set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/include")
  else()
    set(include_dir "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  endif()
  if(NOA_LIBRARY_SOURCES)
    target_include_directories(${TARGET_NAME} PUBLIC
      "$<BUILD_INTERFACE:${include_dir}>"
      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  else()
    target_include_directories(${TARGET_NAME} INTERFACE
      "$<BUILD_INTERFACE:${include_dir}>"
      "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  endif()

  if(NOA_LIBRARY_SOURCES)
    if(NOA_LIBRARY_VARIANT)
      set(export_name "${NOA_LIBRARY_PROJECT}::${NOA_LIBRARY_NAME}::${NOA_LIBRARY_VARIANT}")
    else()
      set(export_name "${NOA_LIBRARY_PROJECT}::${NOA_LIBRARY_NAME}")
    endif()

    set_target_properties(${TARGET_NAME}
      PROPERTIES
        OUTPUT_NAME ${TARGET_NAME}
        PUBLIC_HEADER "${PUBLIC_HEADER}"
        PRIVATE_HEADER "${ABSOLUTE_PRIVATE_HEADERS}"
        EXPORT_NAME "${export_name}"
        FOLDER "${NOA_LIBRARY_FOLDER}")
  else()
    set_target_properties(${TARGET_NAME}
      PROPERTIES
        OUTPUT_NAME ${TARGET_NAME}
        PUBLIC_HEADER "${PUBLIC_HEADER}"
        PRIVATE_HEADER "${ABSOLUTE_PRIVATE_HEADERS}"
        FOLDER "${NOA_LIBRARY_FOLDER}")
  endif()

  if(NOA_LIBRARY_SOURCES)
    include(GenerateExportHeader)
    generate_export_header(${TARGET_NAME}
      EXPORT_FILE_NAME ${EXPORT_HEADER_PATH})
    set_target_properties(${TARGET_NAME}
      PROPERTIES
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        VERSION "${PROJECT_VERSION}")

    # To find the generated files
    target_include_directories(${TARGET_NAME}
      PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")
  endif()
endfunction()

function(noa_library_install)
  cmake_parse_arguments(NOA_LIBRARY "" "NAMESPACE;PROJECT;NAME;VARIANT" "" ${ARGN})

  if(NOT NOA_LIBRARY_PROJECT)
    message(FATAL_ERROR "You must pass the project name using the PROJECT option")
  endif()
  if(NOT NOA_LIBRARY_NAME)
    message(FATAL_ERROR "You must pass the library name using the NAME option")
  endif()

  if(NOA_LIBRARY_NAMESPACE)
    set(COMPONENT_NAME "${NOA_LIBRARY_NAMESPACE}_${NOA_LIBRARY_PROJECT}")
    set(TARGET_NAME "${NOA_LIBRARY_NAMESPACE}_${NOA_LIBRARY_PROJECT}_${NOA_LIBRARY_NAME}")
    set(INCLUDE_PATH "${CMAKE_INSTALL_INCLUDEDIR}/${NOA_LIBRARY_NAMESPACE}/${NOA_LIBRARY_PROJECT}")
    set(NAMESPACE_PREFIX "${NOA_LIBRARY_NAMESPACE}::")
  else()
    set(COMPONENT_NAME "${NOA_LIBRARY_PROJECT}")
    set(TARGET_NAME "${NOA_LIBRARY_PROJECT}_${NOA_LIBRARY_NAME}")
    set(INCLUDE_PATH "${CMAKE_INSTALL_INCLUDEDIR}/${NOA_LIBRARY_PROJECT}")
    set(NAMESPACE_PREFIX "")
  endif()

  if(NOA_LIBRARY_VARIANT)
    set(TARGET_NAME "${TARGET_NAME}_${NOA_LIBRARY_VARIANT}")
  endif()

  include(GNUInstallDirs)
  install(TARGETS ${TARGET_NAME}
    EXPORT ${TARGET_NAME}
    PUBLIC_HEADER DESTINATION "${INCLUDE_PATH}"
      COMPONENT ${COMPONENT_NAME}_dev
    PRIVATE_HEADER DESTINATION "${INCLUDE_PATH}"
      COMPONENT ${COMPONENT_NAME}_dev
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
      COMPONENT ${COMPONENT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      COMPONENT ${COMPONENT_NAME}
      NAMELINK_COMPONENT ${COMPONENT_NAME}_dev
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT ${COMPONENT_NAME}_dev)
  install(EXPORT ${TARGET_NAME}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${NOA_LIBRARY_PROJECT}"
    NAMESPACE ${NAMESPACE_PREFIX}
    COMPONENT ${COMPONENT_NAME}_dev)
endfunction()
