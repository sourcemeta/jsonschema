sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME options
  PRIVATE_HEADERS error.h SOURCES options.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME options)
endif()
