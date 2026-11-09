sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME allocator
  SOURCES allocator.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME allocator)
endif()

if(Mimalloc_FOUND)
  # The standard entry points that this library replaces, defined here so that
  # the interface contract a program gets by linking it is written down in this
  # library rather than left to whichever allocator sits behind it. The half
  # that replaces the platform's own names stays in its language, where every
  # one of them keeps that language's linkage without being asked, and where
  # the diagnostics this project holds its own names to do not reach a set of
  # names it does not get to choose
  target_sources(sourcemeta_core_allocator PRIVATE malloc.c new.cc)

  target_compile_definitions(sourcemeta_core_allocator
    PRIVATE SOURCEMETA_CORE_ALLOCATOR_MIMALLOC)

  # Defining this platform's own names replaces them only for the program that
  # does the defining, so a library that is not the program has to ask the
  # platform to redirect them instead
  if(APPLE AND BUILD_SHARED_LIBS)
    target_compile_definitions(sourcemeta_core_allocator
      PRIVATE SOURCEMETA_CORE_ALLOCATOR_REDIRECT)
  endif()
  target_link_libraries(sourcemeta_core_allocator PRIVATE Mimalloc::Mimalloc)

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    # This library is the allocator, so the compiler cannot be allowed to
    # reason about the entry points it defines or to synthesise calls to them.
    # Each one has to be named, as the option only covers the entry point it
    # names
    target_compile_options(sourcemeta_core_allocator PRIVATE
      -fno-builtin-malloc
      -fno-builtin-calloc
      -fno-builtin-realloc
      -fno-builtin-free)
  endif()
endif()

# Every executable in this build links this library, and no library does, so
# that the standard entry points it replaces are defined exactly once and only
# where a program asked for them
if(BUILD_SHARED_LIBS)
  set(SOURCEMETA_CORE_ALLOCATOR_TARGET sourcemeta_core_allocator CACHE INTERNAL
    "The library that carries the allocator for the executables in this build")
else()
  # An archive hands a linker its members only as something still missing calls
  # for them, so the standard entry points end up taken from whichever archive
  # is reached first and the standard library ships its own. Asking for the
  # whole of this archive is what puts these entry points in place before any
  # of those is looked for, rather than leaving which allocator a program runs
  # on to the order its archives happen to be in
  set(SOURCEMETA_CORE_ALLOCATOR_TARGET
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,sourcemeta_core_allocator>" CACHE INTERNAL
    "The library that carries the allocator for the executables in this build")
endif()
