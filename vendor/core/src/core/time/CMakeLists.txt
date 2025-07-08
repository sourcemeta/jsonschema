sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME time SOURCES gmt.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME time)
endif()
