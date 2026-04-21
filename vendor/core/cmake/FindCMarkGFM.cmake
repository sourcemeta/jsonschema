if(NOT CMarkGFM_FOUND)
  set(CMARK_GFM_DIR "${PROJECT_SOURCE_DIR}/vendor/cmark-gfm")
  set(CMARK_GFM_SOURCE_DIR "${CMARK_GFM_DIR}/src")
  set(CMARK_GFM_EXTENSIONS_DIR "${CMARK_GFM_DIR}/extensions")
  set(CMARK_GFM_BINARY_DIR "${PROJECT_BINARY_DIR}/cmark-gfm")

  file(MAKE_DIRECTORY "${CMARK_GFM_BINARY_DIR}/include")

  set(CMARK_GFM_VERSION_MAJOR 0)
  set(CMARK_GFM_VERSION_MINOR 0)
  set(CMARK_GFM_VERSION_PATCH 0)
  set(CMARK_GFM_VERSION_GFM 0)

  include(CheckIncludeFile)
  include(CheckSymbolExists)
  check_include_file(stdbool.h HAVE_STDBOOL_H)
  check_symbol_exists(__builtin_expect "" HAVE___BUILTIN_EXPECT)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    set(HAVE___ATTRIBUTE__ 1)
  endif()

  configure_file(
    "${CMARK_GFM_SOURCE_DIR}/config.h.in"
    "${CMARK_GFM_BINARY_DIR}/include/config.h")

  set(_SAVED_PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(_SAVED_PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(_SAVED_PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(PROJECT_VERSION_MAJOR ${CMARK_GFM_VERSION_MAJOR})
  set(PROJECT_VERSION_MINOR ${CMARK_GFM_VERSION_MINOR})
  set(PROJECT_VERSION_PATCH ${CMARK_GFM_VERSION_PATCH})
  set(PROJECT_VERSION_GFM ${CMARK_GFM_VERSION_GFM})
  configure_file(
    "${CMARK_GFM_SOURCE_DIR}/cmark-gfm_version.h.in"
    "${CMARK_GFM_BINARY_DIR}/include/cmark-gfm_version.h")
  set(PROJECT_VERSION_MAJOR ${_SAVED_PROJECT_VERSION_MAJOR})
  set(PROJECT_VERSION_MINOR ${_SAVED_PROJECT_VERSION_MINOR})
  set(PROJECT_VERSION_PATCH ${_SAVED_PROJECT_VERSION_PATCH})
  unset(_SAVED_PROJECT_VERSION_MAJOR)
  unset(_SAVED_PROJECT_VERSION_MINOR)
  unset(_SAVED_PROJECT_VERSION_PATCH)

  set(CMARK_GFM_CORE_SOURCES
    "${CMARK_GFM_SOURCE_DIR}/arena.c"
    "${CMARK_GFM_SOURCE_DIR}/blocks.c"
    "${CMARK_GFM_SOURCE_DIR}/buffer.c"
    "${CMARK_GFM_SOURCE_DIR}/cmark.c"
    "${CMARK_GFM_SOURCE_DIR}/cmark_ctype.c"
    "${CMARK_GFM_SOURCE_DIR}/commonmark.c"
    "${CMARK_GFM_SOURCE_DIR}/footnotes.c"
    "${CMARK_GFM_SOURCE_DIR}/houdini_href_e.c"
    "${CMARK_GFM_SOURCE_DIR}/houdini_html_e.c"
    "${CMARK_GFM_SOURCE_DIR}/houdini_html_u.c"
    "${CMARK_GFM_SOURCE_DIR}/html.c"
    "${CMARK_GFM_SOURCE_DIR}/inlines.c"
    "${CMARK_GFM_SOURCE_DIR}/iterator.c"
    "${CMARK_GFM_SOURCE_DIR}/latex.c"
    "${CMARK_GFM_SOURCE_DIR}/linked_list.c"
    "${CMARK_GFM_SOURCE_DIR}/man.c"
    "${CMARK_GFM_SOURCE_DIR}/map.c"
    "${CMARK_GFM_SOURCE_DIR}/node.c"
    "${CMARK_GFM_SOURCE_DIR}/plaintext.c"
    "${CMARK_GFM_SOURCE_DIR}/plugin.c"
    "${CMARK_GFM_SOURCE_DIR}/references.c"
    "${CMARK_GFM_SOURCE_DIR}/registry.c"
    "${CMARK_GFM_SOURCE_DIR}/render.c"
    "${CMARK_GFM_SOURCE_DIR}/scanners.c"
    "${CMARK_GFM_SOURCE_DIR}/syntax_extension.c"
    "${CMARK_GFM_SOURCE_DIR}/utf8.c"
    "${CMARK_GFM_SOURCE_DIR}/xml.c")

  set(CMARK_GFM_EXTENSION_SOURCES
    "${CMARK_GFM_EXTENSIONS_DIR}/autolink.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/core-extensions.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/ext_scanners.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/strikethrough.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/table.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/tagfilter.c"
    "${CMARK_GFM_EXTENSIONS_DIR}/tasklist.c")

  add_library(cmark_gfm
    ${CMARK_GFM_CORE_SOURCES} ${CMARK_GFM_EXTENSION_SOURCES})
  sourcemeta_add_default_options(PRIVATE cmark_gfm)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(cmark_gfm PRIVATE -Wno-sign-conversion)
    target_compile_options(cmark_gfm PRIVATE -Wno-unused-parameter)
  endif()

  if(SOURCEMETA_COMPILER_MSVC)
    target_compile_options(cmark_gfm PRIVATE /wd4100)
    target_compile_definitions(cmark_gfm PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()

  target_include_directories(cmark_gfm PRIVATE
    "${CMARK_GFM_BINARY_DIR}/include"
    "${CMARK_GFM_SOURCE_DIR}"
    "${CMARK_GFM_EXTENSIONS_DIR}")

  target_include_directories(cmark_gfm PUBLIC
    "$<BUILD_INTERFACE:${CMARK_GFM_BINARY_DIR}/include>"
    "$<BUILD_INTERFACE:${CMARK_GFM_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMARK_GFM_EXTENSIONS_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_compile_definitions(cmark_gfm PRIVATE HAVE_CONFIG_H)

  include(GenerateExportHeader)
  generate_export_header(cmark_gfm
    BASE_NAME CMARK_GFM
    EXPORT_FILE_NAME "${CMARK_GFM_BINARY_DIR}/include/cmark-gfm_export.h")

  add_library(CMarkGFM::cmark_gfm ALIAS cmark_gfm)

  set_target_properties(cmark_gfm
    PROPERTIES
      OUTPUT_NAME cmark_gfm
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      EXPORT_NAME cmark_gfm
      WINDOWS_EXPORT_ALL_SYMBOLS OFF)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS cmark_gfm
      EXPORT cmark_gfm
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT cmark_gfm
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cmark_gfm"
      NAMESPACE CMarkGFM::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmark_gfm-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/cmark_gfm.cmake\")\n"
      "check_required_components(\"cmark_gfm\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cmark_gfm-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cmark_gfm"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(CMarkGFM_FOUND ON)
endif()
