sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME text
  PRIVATE_HEADERS static_string.h
  SOURCES text.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME text)
endif()
