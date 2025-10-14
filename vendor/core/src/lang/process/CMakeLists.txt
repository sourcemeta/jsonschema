sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME process
  PRIVATE_HEADERS error.h
  SOURCES spawn.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME process)
endif()
