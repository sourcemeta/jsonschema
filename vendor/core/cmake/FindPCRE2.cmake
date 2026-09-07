if(NOT PCRE2_FOUND)
  set(PCRE2_DIR "${PROJECT_SOURCE_DIR}/vendor/pcre2")
  set(PCRE2_SOURCE_DIR "${PCRE2_DIR}/src")
  set(PCRE2_BINARY_DIR "${PROJECT_BINARY_DIR}/pcre2")

  file(MAKE_DIRECTORY "${PCRE2_BINARY_DIR}/interface")
  file(MAKE_DIRECTORY "${PCRE2_BINARY_DIR}/src")

  set(HAVE_ASSERT_H 1)
  set(HAVE_SYS_STAT_H 1)
  set(HAVE_SYS_TYPES_H 1)

  if(WIN32)
    set(HAVE_WINDOWS_H 1)
  else()
    set(HAVE_DIRENT_H 1)
    set(HAVE_UNISTD_H 1)
  endif()

  set(PCRE2_MAJOR 0)
  set(PCRE2_MINOR 0)
  set(PCRE2_PRERELEASE "")
  set(PCRE2_DATE "0000-00-00")

  set(PCRE2_LINK_SIZE 2)
  set(PCRE2_PARENS_NEST_LIMIT 250)
  set(PCRE2_HEAP_LIMIT 20000000)
  set(PCRE2_MAX_VARLOOKBEHIND 255)
  set(PCRE2_MATCH_LIMIT 10000000)
  set(PCRE2_MATCH_LIMIT_DEPTH MATCH_LIMIT)
  set(PCRE2GREP_BUFSIZE 20480)
  set(PCRE2GREP_MAX_BUFSIZE 1048576)
  set(NEWLINE_DEFAULT 2)

  # This library is compiled into the one that uses it rather than shipped
  # on its own, so its symbols must never leave the resulting binary
  set(PCRE2_EXPORT)

  set(SUPPORT_PCRE2_8 1)
  set(SUPPORT_UNICODE 1)
  set(SUPPORT_JIT 1)

  configure_file(
    "${PCRE2_SOURCE_DIR}/pcre2.h.in"
    "${PCRE2_BINARY_DIR}/interface/pcre2.h"
    @ONLY)

  configure_file(
    "${PCRE2_SOURCE_DIR}/config-cmake.h.in"
    "${PCRE2_BINARY_DIR}/src/config.h"
    @ONLY)

  configure_file(
    "${PCRE2_SOURCE_DIR}/pcre2_chartables.c.dist"
    "${PCRE2_BINARY_DIR}/src/pcre2_chartables.c"
    COPYONLY)

  set(PCRE2_SOURCES
    "${PCRE2_SOURCE_DIR}/pcre2_auto_possess.c"
    "${PCRE2_BINARY_DIR}/src/pcre2_chartables.c"
    "${PCRE2_SOURCE_DIR}/pcre2_chkdint.c"
    "${PCRE2_SOURCE_DIR}/pcre2_compile.c"
    "${PCRE2_SOURCE_DIR}/pcre2_compile_cgroup.c"
    "${PCRE2_SOURCE_DIR}/pcre2_compile_class.c"
    "${PCRE2_SOURCE_DIR}/pcre2_config.c"
    "${PCRE2_SOURCE_DIR}/pcre2_context.c"
    "${PCRE2_SOURCE_DIR}/pcre2_convert.c"
    "${PCRE2_SOURCE_DIR}/pcre2_dfa_match.c"
    "${PCRE2_SOURCE_DIR}/pcre2_error.c"
    "${PCRE2_SOURCE_DIR}/pcre2_extuni.c"
    "${PCRE2_SOURCE_DIR}/pcre2_find_bracket.c"
    "${PCRE2_SOURCE_DIR}/pcre2_jit_compile.c"
    "${PCRE2_SOURCE_DIR}/pcre2_maketables.c"
    "${PCRE2_SOURCE_DIR}/pcre2_match.c"
    "${PCRE2_SOURCE_DIR}/pcre2_match_data.c"
    "${PCRE2_SOURCE_DIR}/pcre2_match_next.c"
    "${PCRE2_SOURCE_DIR}/pcre2_newline.c"
    "${PCRE2_SOURCE_DIR}/pcre2_ord2utf.c"
    "${PCRE2_SOURCE_DIR}/pcre2_pattern_info.c"
    "${PCRE2_SOURCE_DIR}/pcre2_script_run.c"
    "${PCRE2_SOURCE_DIR}/pcre2_serialize.c"
    "${PCRE2_SOURCE_DIR}/pcre2_string_utils.c"
    "${PCRE2_SOURCE_DIR}/pcre2_study.c"
    "${PCRE2_SOURCE_DIR}/pcre2_substitute.c"
    "${PCRE2_SOURCE_DIR}/pcre2_substring.c"
    "${PCRE2_SOURCE_DIR}/pcre2_tables.c"
    "${PCRE2_SOURCE_DIR}/pcre2_ucd.c"
    "${PCRE2_SOURCE_DIR}/pcre2_valid_utf.c"
    "${PCRE2_SOURCE_DIR}/pcre2_xclass.c")

  # Merged into the library that uses it, so that no archive, no header and
  # no CMake package of our own build of it reaches an installed consumer
  add_library(pcre2 OBJECT ${PCRE2_SOURCES})
  sourcemeta_add_default_options(PRIVATE pcre2)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(pcre2 PRIVATE -Wno-implicit-int-conversion)
    target_compile_options(pcre2 PRIVATE -Wno-sign-conversion)
    target_compile_options(pcre2 PRIVATE -Wno-comma)
    target_compile_options(pcre2 PRIVATE -Wno-conditional-uninitialized)
    target_compile_options(pcre2 PRIVATE -Wno-overlength-strings)
    target_compile_options(pcre2 PRIVATE -Wno-conversion)
    target_compile_options(pcre2 PRIVATE -Wno-type-limits)
    target_compile_options(pcre2 PRIVATE -fstrict-flex-arrays=0)
  endif()

  if(SOURCEMETA_COMPILER_MSVC)
    target_compile_options(pcre2 PRIVATE /sdl-)
    target_compile_options(pcre2 PRIVATE /wd4127)
    target_compile_options(pcre2 PRIVATE /wd4244)
    target_compile_options(pcre2 PRIVATE /wd4267)
    target_compile_options(pcre2 PRIVATE /wd4389)
    target_compile_options(pcre2 PRIVATE /wd4701)
    target_compile_options(pcre2 PRIVATE /wd4702)
  endif()

  target_include_directories(pcre2 PRIVATE
    "${PCRE2_BINARY_DIR}/interface"
    "${PCRE2_BINARY_DIR}/src"
    "${PCRE2_SOURCE_DIR}")

  target_include_directories(pcre2 PUBLIC
    "$<BUILD_INTERFACE:${PCRE2_BINARY_DIR}/interface>")

  target_compile_definitions(pcre2 PRIVATE HAVE_CONFIG_H)
  target_compile_definitions(pcre2 PUBLIC PCRE2_CODE_UNIT_WIDTH=8)
  target_compile_definitions(pcre2 PRIVATE SUPPORT_PCRE2_8=1)
  target_compile_definitions(pcre2 PRIVATE SUPPORT_UNICODE=1)
  # The just-in-time compiler brings its own code generator in as part of one
  # of its translation units, so there is no second library to build for it
  target_compile_definitions(pcre2 PRIVATE SUPPORT_JIT=1)
  # Declarations of an import from a shared library of our own build of this
  # one, which no longer exists, would be unresolvable on Windows
  target_compile_definitions(pcre2 PUBLIC PCRE2_STATIC=1)

  if(SOURCEMETA_OS_LINUX)
    target_compile_definitions(pcre2 PRIVATE _GNU_SOURCE)
  endif()

  add_library(PCRE2::pcre2 ALIAS pcre2)

  set(PCRE2_FOUND ON)
endif()
