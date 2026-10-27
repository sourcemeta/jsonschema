sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME allocator
  SOURCES allocator.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME allocator)
endif()

if(Mimalloc_FOUND)
  target_compile_definitions(sourcemeta_core_allocator
    PRIVATE SOURCEMETA_CORE_ALLOCATOR_MIMALLOC)
  target_link_libraries(sourcemeta_core_allocator PRIVATE Mimalloc::Mimalloc)
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
