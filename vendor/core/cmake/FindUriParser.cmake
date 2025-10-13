if(NOT UriParser_FOUND)
  set(URIPARSER_DIR "${PROJECT_SOURCE_DIR}/vendor/uriparser")

  set(URIPARSER_PUBLIC_HEADER "${URIPARSER_DIR}/include/uriparser/Uri.h")
  set(URIPARSER_PRIVATE_HEADERS
    "${URIPARSER_DIR}/include/uriparser/UriBase.h"
    "${URIPARSER_DIR}/include/uriparser/UriDefsAnsi.h"
    "${URIPARSER_DIR}/include/uriparser/UriDefsConfig.h"
    "${URIPARSER_DIR}/include/uriparser/UriDefsUnicode.h"
    "${URIPARSER_DIR}/include/uriparser/UriIp4.h")

  configure_file("${URIPARSER_DIR}/src/UriConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uriparser/UriConfig.h")

  set(URIPARSER_SOURCES
    "${URIPARSER_PUBLIC_HEADER}" ${URIPARSER_PRIVATE_HEADERS}
    "${CMAKE_CURRENT_BINARY_DIR}/uriparser/UriConfig.h"
    "${URIPARSER_DIR}/src/UriCommon.c"
    "${URIPARSER_DIR}/src/UriCommon.h"
    "${URIPARSER_DIR}/src/UriCompare.c"
    "${URIPARSER_DIR}/src/UriEscape.c"
    "${URIPARSER_DIR}/src/UriFile.c"
    "${URIPARSER_DIR}/src/UriIp4.c"
    "${URIPARSER_DIR}/src/UriIp4Base.c"
    "${URIPARSER_DIR}/src/UriIp4Base.h"
    "${URIPARSER_DIR}/src/UriMemory.c"
    "${URIPARSER_DIR}/src/UriMemory.h"
    "${URIPARSER_DIR}/src/UriNormalize.c"
    "${URIPARSER_DIR}/src/UriNormalizeBase.c"
    "${URIPARSER_DIR}/src/UriNormalizeBase.h"
    "${URIPARSER_DIR}/src/UriParse.c"
    "${URIPARSER_DIR}/src/UriParseBase.c"
    "${URIPARSER_DIR}/src/UriParseBase.h"
    "${URIPARSER_DIR}/src/UriQuery.c"
    "${URIPARSER_DIR}/src/UriRecompose.c"
    "${URIPARSER_DIR}/src/UriResolve.c"
    "${URIPARSER_DIR}/src/UriShorten.c"
    "${URIPARSER_DIR}/src/UriCopy.c"
    "${URIPARSER_DIR}/src/UriCopy.h"
    "${URIPARSER_DIR}/src/UriNormalize.h"
    "${URIPARSER_DIR}/src/UriSetFragment.c"
    "${URIPARSER_DIR}/src/UriSetHostAuto.c"
    "${URIPARSER_DIR}/src/UriSetHostBase.h"
    "${URIPARSER_DIR}/src/UriSetHostCommon.c"
    "${URIPARSER_DIR}/src/UriSetHostCommon.h"
    "${URIPARSER_DIR}/src/UriSetHostIp4.c"
    "${URIPARSER_DIR}/src/UriSetHostIp6.c"
    "${URIPARSER_DIR}/src/UriSetHostIpFuture.c"
    "${URIPARSER_DIR}/src/UriSetHostRegName.c"
    "${URIPARSER_DIR}/src/UriSetPath.c"
    "${URIPARSER_DIR}/src/UriSetPort.c"
    "${URIPARSER_DIR}/src/UriSetQuery.c"
    "${URIPARSER_DIR}/src/UriSetScheme.c"
    "${URIPARSER_DIR}/src/UriSetUserInfo.c"
    "${URIPARSER_DIR}/src/UriVersion.c")

  add_library(uriparser ${URIPARSER_SOURCES})
  add_library(uriparser::uriparser ALIAS uriparser)

  if(BUILD_SHARED_LIBS)
    target_compile_definitions(uriparser PUBLIC URI_LIBRARY_BUILD)
    target_compile_definitions(uriparser PUBLIC URI_VISIBILITY)
  else()
    target_compile_definitions(uriparser PUBLIC URI_STATIC_BUILD)
  endif()

  target_include_directories(uriparser PRIVATE "${URIPARSER_DIR}/include")
  target_include_directories(uriparser PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/uriparser")

  target_include_directories(uriparser PUBLIC
    "$<BUILD_INTERFACE:${URIPARSER_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  set_target_properties(uriparser
    PROPERTIES
      OUTPUT_NAME uriparser
      PUBLIC_HEADER "${URIPARSER_PUBLIC_HEADER}"
      PRIVATE_HEADER "${URIPARSER_PRIVATE_HEADERS}"
      EXPORT_NAME uriparser)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS uriparser
      EXPORT uriparser
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/uriparser"
        COMPONENT sourcemeta_core_dev
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/uriparser"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT uriparser
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/uriparser"
      NAMESPACE uriparser::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/uriparser-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/uriparser.cmake\")\n"
      "check_required_components(\"uriparser\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/uriparser-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/uriparser"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(UriParser_FOUND ON)
endif()
