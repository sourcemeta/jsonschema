sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME langtag
  SOURCES langtag.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME langtag)
endif()

target_link_libraries(sourcemeta_core_langtag
  PRIVATE sourcemeta::core::text)
