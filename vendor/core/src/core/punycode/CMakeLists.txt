sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME punycode
  PRIVATE_HEADERS error.h
  SOURCES punycode.cc utf8.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME punycode)
endif()
