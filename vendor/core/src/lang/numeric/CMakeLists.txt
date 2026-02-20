sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME numeric
  PRIVATE_HEADERS error.h decimal.h parse.h uint128.h
  SOURCES decimal.cc parse.cc big_coefficient.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME numeric)
endif()

