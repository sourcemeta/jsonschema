sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME dns
  SOURCES hostname.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME dns)
endif()
