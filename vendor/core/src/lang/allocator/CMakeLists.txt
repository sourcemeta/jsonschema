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
set(SOURCEMETA_CORE_ALLOCATOR_TARGET sourcemeta_core_allocator CACHE INTERNAL
  "The library that carries the allocator for the executables in this build")
