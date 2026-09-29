if(NOT SLJIT_FOUND)
  set(SLJIT_DIR "${PROJECT_SOURCE_DIR}/vendor/sljit")
  set(SLJIT_SOURCE_DIR "${SLJIT_DIR}/sljit_src")

  # This library ships one source that includes every backend and allocator
  # that the target architecture and platform select
  # Merged into the library that uses it, so that no archive, no header and
  # no CMake package of our own build of it reaches an installed consumer
  add_library(sljit OBJECT "${SLJIT_SOURCE_DIR}/sljitLir.c")
  sourcemeta_add_default_options(PRIVATE sljit)

  # The compiler structure that this library hands out grows extra members
  # under any of the tracing, argument checking, or debugging options, so
  # every translation unit that reaches for the header has to be told the
  # same configuration that the library itself was built with
  target_compile_definitions(sljit PUBLIC SLJIT_CONFIG_AUTO=1)
  target_compile_definitions(sljit PUBLIC SLJIT_VERBOSE=0)
  target_compile_definitions(sljit PUBLIC SLJIT_DEBUG=0)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    # Generated code accumulates into a trailing single-element array that is
    # over-allocated and written well past its first element, so the strictest
    # interpretation of what counts as a trailing flexible array would treat
    # every byte this library emits as running off the end of the object
    target_compile_options(sljit PRIVATE -fstrict-flex-arrays=0)
  endif()

  if(SOURCEMETA_COMPILER_LLVM)
    # The immediate byte of a vector lane instruction is only read back on the
    # paths that set it, which the compiler cannot correlate
    target_compile_options(sljit PRIVATE -Wno-conditional-uninitialized)
  endif()

  if(SOURCEMETA_COMPILER_MSVC)
    target_compile_options(sljit PRIVATE /wd4701)
  endif()

  target_include_directories(sljit PUBLIC
    "$<BUILD_INTERFACE:${SLJIT_SOURCE_DIR}>")

  add_library(SLJIT::sljit ALIAS sljit)

  set(SLJIT_FOUND ON)
endif()
