if(NOT PSL_FOUND)
  set(LIBPSL_DIR "${PROJECT_SOURCE_DIR}/vendor/libpsl")

  file(STRINGS "${LIBPSL_DIR}/version.txt" LIBPSL_VERSION_LINE LIMIT_COUNT 1)
  if(LIBPSL_VERSION_LINE)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" LIBPSL_VERSION_MATCH "${LIBPSL_VERSION_LINE}")
    set(LIBPSL_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(LIBPSL_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(LIBPSL_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(LIBPSL_VERSION_STRING "${LIBPSL_VERSION_MAJOR}.${LIBPSL_VERSION_MINOR}.${LIBPSL_VERSION_PATCH}")
    math(EXPR LIBPSL_VERSION_NUMBER
      "(${LIBPSL_VERSION_MAJOR} << 16) | (${LIBPSL_VERSION_MINOR} << 8) | ${LIBPSL_VERSION_PATCH}"
      OUTPUT_FORMAT HEXADECIMAL)
  else()
    message(FATAL_ERROR "Could not find libpsl version in version.txt")
  endif()

  set(LIBPSL_PUBLIC_HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/libpsl_generated/libpsl.h")

  set(LIBPSL_SOURCES
    "${LIBPSL_DIR}/src/psl.c"
    "${LIBPSL_DIR}/src/lookup_string_in_fixed_set.c")

  add_library(psl ${LIBPSL_SOURCES})

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(psl PRIVATE /W3 /wd4996 /wd4267 /wd4244 /MP)
    target_compile_definitions(psl PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(psl PRIVATE
      -Wall
      -Wextra
      -Wno-unused-parameter
      -Wno-sign-conversion
      -Wno-sign-compare
      -Wno-shadow
      -Wno-unused-variable
      -Wno-unused-function
      -Wno-char-subscripts)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(psl PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()
  endif()

  configure_file("${LIBPSL_DIR}/include/libpsl.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libpsl_generated/libpsl.h" @ONLY)

  target_compile_definitions(psl PRIVATE BUILDING_PSL)
  target_compile_definitions(psl PRIVATE "PACKAGE_VERSION=\"${LIBPSL_VERSION_STRING}\"")
  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(psl PUBLIC PSL_STATIC)
  endif()

  if(UNIX)
    target_compile_definitions(psl PRIVATE HAVE_VISIBILITY)
    target_compile_definitions(psl PRIVATE _GNU_SOURCE)
  endif()

  target_include_directories(psl PRIVATE "${LIBPSL_DIR}/src")
  target_include_directories(psl PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libpsl_generated>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  if(WIN32)
    target_link_libraries(psl PRIVATE ws2_32)
  endif()

  add_library(PSL::psl ALIAS psl)

  set_target_properties(psl
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME psl
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME psl)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS psl
      EXPORT psl
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT psl
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/psl"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/psl-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/psl.cmake\")\n"
      "check_required_components(\"psl\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/psl-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/psl"
      COMPONENT sourcemeta_hydra_dev)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libpsl_generated/libpsl.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(PSL_FOUND ON)
endif()
