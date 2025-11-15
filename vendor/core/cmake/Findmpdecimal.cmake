if(NOT mpdecimal_FOUND)
  set(MPDECIMAL_DIR "${PROJECT_SOURCE_DIR}/vendor/mpdecimal")
  set(MPDECIMAL_SOURCE_DIR "${MPDECIMAL_DIR}/libmpdec")
  set(MPDECIMAL_BINARY_DIR "${PROJECT_BINARY_DIR}/mpdecimal")

  file(MAKE_DIRECTORY "${MPDECIMAL_BINARY_DIR}/include")

  if(MSVC)
    configure_file(
      "${MPDECIMAL_SOURCE_DIR}/mpdecimal64vc.h"
      "${MPDECIMAL_BINARY_DIR}/include/mpdecimal.h"
      COPYONLY)
    set(MPD_CONFIG_LIST CONFIG_64 MASM)
  else()
    set(MPD_HEADER_CONFIG "/* ABI: 64-bit */")

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
      set(MPD_CONFIG_LIST CONFIG_64 ASM)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
      set(MPD_CONFIG_LIST CONFIG_64 ANSI HAVE_UINT128_T)
    else()
      set(MPD_CONFIG_LIST CONFIG_64 ANSI)
    endif()

    configure_file(
      "${MPDECIMAL_SOURCE_DIR}/mpdecimal.h.in"
      "${MPDECIMAL_BINARY_DIR}/include/mpdecimal.h"
      @ONLY)
  endif()

  set(MPDECIMAL_PUBLIC_HEADER "${MPDECIMAL_BINARY_DIR}/include/mpdecimal.h")

  set(MPDECIMAL_SOURCES
    "${MPDECIMAL_SOURCE_DIR}/basearith.c"
    "${MPDECIMAL_SOURCE_DIR}/constants.c"
    "${MPDECIMAL_SOURCE_DIR}/context.c"
    "${MPDECIMAL_SOURCE_DIR}/convolute.c"
    "${MPDECIMAL_SOURCE_DIR}/crt.c"
    "${MPDECIMAL_SOURCE_DIR}/difradix2.c"
    "${MPDECIMAL_SOURCE_DIR}/fnt.c"
    "${MPDECIMAL_SOURCE_DIR}/fourstep.c"
    "${MPDECIMAL_SOURCE_DIR}/io.c"
    "${MPDECIMAL_SOURCE_DIR}/mpalloc.c"
    "${MPDECIMAL_SOURCE_DIR}/mpdecimal.c"
    "${MPDECIMAL_SOURCE_DIR}/mpsignal.c"
    "${MPDECIMAL_SOURCE_DIR}/numbertheory.c"
    "${MPDECIMAL_SOURCE_DIR}/sixstep.c"
    "${MPDECIMAL_SOURCE_DIR}/transpose.c")

  if(MSVC)
    list(APPEND MPDECIMAL_SOURCES "${MPDECIMAL_SOURCE_DIR}/vcdiv64.asm")
  endif()

  add_library(mpdecimal ${MPDECIMAL_SOURCES})
  sourcemeta_add_default_options(PRIVATE mpdecimal)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(mpdecimal PRIVATE -Wno-sign-conversion)
    target_compile_options(mpdecimal PRIVATE -Wno-implicit-fallthrough)
    target_compile_options(mpdecimal PRIVATE -Wno-conversion)
  endif()

  if(SOURCEMETA_COMPILER_MSVC)
    target_compile_options(mpdecimal PRIVATE /wd4200)
    target_compile_options(mpdecimal PRIVATE /wd4702)
    target_compile_options(mpdecimal PRIVATE /wd4996)
  endif()

  target_include_directories(mpdecimal PRIVATE
    "${MPDECIMAL_SOURCE_DIR}")

  target_include_directories(mpdecimal PUBLIC
    "$<BUILD_INTERFACE:${MPDECIMAL_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_compile_definitions(mpdecimal PUBLIC MPD_CONFIG_64)
  foreach(config_item ${MPD_CONFIG_LIST})
    target_compile_definitions(mpdecimal PRIVATE ${config_item})
  endforeach()

  target_compile_definitions(mpdecimal PRIVATE NDEBUG)

  if(SOURCEMETA_OS_LINUX)
    target_compile_definitions(mpdecimal PRIVATE _GNU_SOURCE)
  endif()

  if(UNIX AND NOT APPLE)
    target_link_libraries(mpdecimal PRIVATE m)
  endif()

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(mpdecimal PRIVATE -Wall -Wextra -Wno-unknown-pragmas)
    if(BUILD_SHARED_LIBS)
      target_compile_options(mpdecimal PUBLIC -fvisibility=default)
    endif()
  endif()

  if(MSVC)
    if(BUILD_SHARED_LIBS)
      target_compile_definitions(mpdecimal PRIVATE BUILD_LIBMPDEC)
    else()
      target_compile_options(mpdecimal PRIVATE /wd4273)
      target_compile_definitions(mpdecimal PUBLIC BUILD_LIBMPDEC)
    endif()
  endif()

  add_library(mpdecimal::mpdecimal ALIAS mpdecimal)

  set_target_properties(mpdecimal
    PROPERTIES
      OUTPUT_NAME mpdecimal
      PUBLIC_HEADER "${MPDECIMAL_PUBLIC_HEADER}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      POSITION_INDEPENDENT_CODE ON
      EXPORT_NAME mpdecimal)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS mpdecimal
      EXPORT mpdecimal
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT mpdecimal
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mpdecimal"
      NAMESPACE mpdecimal::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/mpdecimal-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/mpdecimal.cmake\")\n"
      "check_required_components(\"mpdecimal\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/mpdecimal-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mpdecimal"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(mpdecimal_FOUND ON)
endif()
