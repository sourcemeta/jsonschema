sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME html
  PRIVATE_HEADERS escape.h encoder.h elements.h
  SOURCES escape.cc encoder.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME html)
endif()
