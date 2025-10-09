if(NOT Nghttp2_FOUND)
  set(NGHTTP2_DIR "${PROJECT_SOURCE_DIR}/vendor/nghttp2")

  file(STRINGS "${PROJECT_SOURCE_DIR}/DEPENDENCIES" NGHTTP2_VERSION_LINE
    REGEX "^nghttp2 ")
  if(NGHTTP2_VERSION_LINE)
    string(REGEX MATCH "v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" NGHTTP2_VERSION_MATCH "${NGHTTP2_VERSION_LINE}")
    set(NGHTTP2_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(NGHTTP2_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(NGHTTP2_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(NGHTTP2_VERSION_STRING "${NGHTTP2_VERSION_MAJOR}.${NGHTTP2_VERSION_MINOR}.${NGHTTP2_VERSION_PATCH}")
    math(EXPR NGHTTP2_VERSION_NUM
      "(${NGHTTP2_VERSION_MAJOR} << 16) | (${NGHTTP2_VERSION_MINOR} << 8) | ${NGHTTP2_VERSION_PATCH}"
      OUTPUT_FORMAT HEXADECIMAL)
  else()
    message(FATAL_ERROR "Could not find nghttp2 version in DEPENDENCIES")
  endif()

  set(NGHTTP2_PUBLIC_HEADER "${NGHTTP2_DIR}/lib/includes/nghttp2/nghttp2.h")

  set(NGHTTP2_SOURCES
    "${NGHTTP2_DIR}/lib/nghttp2_alpn.c"
    "${NGHTTP2_DIR}/lib/nghttp2_buf.c"
    "${NGHTTP2_DIR}/lib/nghttp2_callbacks.c"
    "${NGHTTP2_DIR}/lib/nghttp2_debug.c"
    "${NGHTTP2_DIR}/lib/nghttp2_extpri.c"
    "${NGHTTP2_DIR}/lib/nghttp2_frame.c"
    "${NGHTTP2_DIR}/lib/nghttp2_hd.c"
    "${NGHTTP2_DIR}/lib/nghttp2_hd_huffman.c"
    "${NGHTTP2_DIR}/lib/nghttp2_hd_huffman_data.c"
    "${NGHTTP2_DIR}/lib/nghttp2_helper.c"
    "${NGHTTP2_DIR}/lib/nghttp2_http.c"
    "${NGHTTP2_DIR}/lib/nghttp2_map.c"
    "${NGHTTP2_DIR}/lib/nghttp2_mem.c"
    "${NGHTTP2_DIR}/lib/nghttp2_option.c"
    "${NGHTTP2_DIR}/lib/nghttp2_outbound_item.c"
    "${NGHTTP2_DIR}/lib/nghttp2_pq.c"
    "${NGHTTP2_DIR}/lib/nghttp2_priority_spec.c"
    "${NGHTTP2_DIR}/lib/nghttp2_queue.c"
    "${NGHTTP2_DIR}/lib/nghttp2_ratelim.c"
    "${NGHTTP2_DIR}/lib/nghttp2_rcbuf.c"
    "${NGHTTP2_DIR}/lib/nghttp2_session.c"
    "${NGHTTP2_DIR}/lib/nghttp2_stream.c"
    "${NGHTTP2_DIR}/lib/nghttp2_submit.c"
    "${NGHTTP2_DIR}/lib/nghttp2_time.c"
    "${NGHTTP2_DIR}/lib/nghttp2_version.c"
    "${NGHTTP2_DIR}/lib/sfparse.c")

  add_library(nghttp2 ${NGHTTP2_SOURCES})

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(nghttp2 PRIVATE
      /W3 /wd4996 /wd4334 /MP /Zc:inline- /GS-)
    target_compile_definitions(nghttp2 PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Define ssize_t for MSVC using the same approach as curl's config-win32.h
    # This ensures compatibility between nghttp2 and curl
    target_compile_definitions(nghttp2 PUBLIC _SSIZE_T_DEFINED)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      target_compile_definitions(nghttp2 PUBLIC "ssize_t=__int64")
    else()
      target_compile_definitions(nghttp2 PUBLIC "ssize_t=int")
    endif()
  else()
    target_compile_options(nghttp2 PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wdouble-promotion
      -Wfloat-equal
      -Wmissing-declarations
      -Wwrite-strings
      -Wno-cast-align
      -Wno-cast-qual
      -Wno-format-nonliteral
      -Wno-unused-parameter
      -Wno-sign-conversion
      -Wno-implicit-int-conversion
      -Wno-sign-compare
      -Wno-shadow
      -Wno-implicit-function-declaration
      -Wno-strict-overflow)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(nghttp2 PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()
  endif()

  set(PACKAGE_VERSION "${NGHTTP2_VERSION_STRING}")
  set(PACKAGE_VERSION_NUM "${NGHTTP2_VERSION_NUM}")
  configure_file("${NGHTTP2_DIR}/lib/includes/nghttp2/nghttp2ver.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/nghttp2_generated/nghttp2/nghttp2ver.h" @ONLY)

  target_compile_definitions(nghttp2 PRIVATE BUILDING_NGHTTP2)
  target_compile_definitions(nghttp2 PRIVATE NGHTTP2_STATICLIB)
  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(nghttp2 PUBLIC NGHTTP2_STATICLIB)
  endif()

  target_include_directories(nghttp2 PRIVATE "${NGHTTP2_DIR}/lib")
  target_include_directories(nghttp2 PUBLIC
    "$<BUILD_INTERFACE:${NGHTTP2_DIR}/lib/includes>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/nghttp2_generated>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(Nghttp2::nghttp2 ALIAS nghttp2)

  set_target_properties(nghttp2
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME nghttp2
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME nghttp2)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS nghttp2
      EXPORT nghttp2
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT nghttp2
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nghttp2"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/nghttp2-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/nghttp2.cmake\")\n"
      "check_required_components(\"nghttp2\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/nghttp2-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nghttp2"
      COMPONENT sourcemeta_hydra_dev)

    install(FILES "${NGHTTP2_PUBLIC_HEADER}"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nghttp2"
      COMPONENT sourcemeta_hydra_dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nghttp2_generated/nghttp2/nghttp2ver.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/nghttp2"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(Nghttp2_FOUND ON)
endif()
