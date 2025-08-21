sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME parallel
  PRIVATE_HEADERS for_each.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME parallel)
endif()

target_link_libraries(sourcemeta_core_parallel INTERFACE Threads::Threads)
