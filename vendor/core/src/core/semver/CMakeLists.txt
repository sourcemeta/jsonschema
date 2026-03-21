sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME semver
  PRIVATE_HEADERS error.h
  SOURCES semver.cc)
target_link_libraries(sourcemeta_core_semver
  PUBLIC sourcemeta::core::preprocessor)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME semver)
endif()
