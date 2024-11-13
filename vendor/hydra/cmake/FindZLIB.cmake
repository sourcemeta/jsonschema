if(NOT ZLIB_FOUND)
  set(Z_HAVE_UNISTD_H ON)
  configure_file("${PROJECT_SOURCE_DIR}/vendor/zlib/zconf.h.cmakein"
    "${CMAKE_CURRENT_BINARY_DIR}/zlib/zconf.h" @ONLY)

  set(ZLIB_DIR "${PROJECT_SOURCE_DIR}/vendor/zlib")
  set(ZLIB_PUBLIC_HEADER "${ZLIB_DIR}/zlib.h")
  set(ZLIB_PRIVATE_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/zlib/zconf.h")

  add_library(zlib
    "${ZLIB_PUBLIC_HEADER}" ${ZLIB_PRIVATE_HEADERS}
    "${ZLIB_DIR}/adler32.c"
    "${ZLIB_DIR}/compress.c"
    "${ZLIB_DIR}/crc32.c"
    "${ZLIB_DIR}/crc32.h"
    "${ZLIB_DIR}/deflate.c"
    "${ZLIB_DIR}/deflate.h"
    "${ZLIB_DIR}/gzclose.c"
    "${ZLIB_DIR}/gzguts.h"
    "${ZLIB_DIR}/gzlib.c"
    "${ZLIB_DIR}/gzread.c"
    "${ZLIB_DIR}/gzwrite.c"
    "${ZLIB_DIR}/infback.c"
    "${ZLIB_DIR}/inffast.c"
    "${ZLIB_DIR}/inffast.h"
    "${ZLIB_DIR}/inffixed.h"
    "${ZLIB_DIR}/inflate.c"
    "${ZLIB_DIR}/inflate.h"
    "${ZLIB_DIR}/inftrees.c"
    "${ZLIB_DIR}/inftrees.h"
    "${ZLIB_DIR}/trees.c"
    "${ZLIB_DIR}/trees.h"
    "${ZLIB_DIR}/uncompr.c"
    "${ZLIB_DIR}/zutil.c"
    "${ZLIB_DIR}/zutil.h")

  target_compile_definitions(zlib PUBLIC NO_FSEEKO)
  target_compile_definitions(zlib PUBLIC _LARGEFILE64_SOURCE=1)

  target_include_directories(zlib PUBLIC
    "$<BUILD_INTERFACE:${ZLIB_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(ZLIB::ZLIB ALIAS zlib)

  set_target_properties(zlib
    PROPERTIES
      OUTPUT_NAME zlib
      PUBLIC_HEADER "${ZLIB_PUBLIC_HEADER}"
      PRIVATE_HEADER "${ZLIB_PRIVATE_HEADERS}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME zlib)

  if(HYDRA_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS zlib
      EXPORT zlib
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_hydra_dev
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_hydra_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT zlib
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zlib"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/zlib-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/zlib.cmake\")\n"
      "check_required_components(\"zlib\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/zlib-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zlib"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(ZLIB_FOUND ON)
endif()
