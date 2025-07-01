sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME uuid SOURCES uuid.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME uuid)
endif()
