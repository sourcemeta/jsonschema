sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME diff
  SOURCES diff.cc myers.h stringify.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME diff)
endif()

target_link_libraries(sourcemeta_core_diff PRIVATE sourcemeta::core::text)
