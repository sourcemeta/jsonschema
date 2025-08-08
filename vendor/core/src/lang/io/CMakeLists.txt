sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME io SOURCES io.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME io)
endif()
