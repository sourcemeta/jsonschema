sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME unicode
  SOURCES unicode.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME unicode)
endif()
