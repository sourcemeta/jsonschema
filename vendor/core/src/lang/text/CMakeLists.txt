sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME text
  SOURCES text.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME text)
endif()
