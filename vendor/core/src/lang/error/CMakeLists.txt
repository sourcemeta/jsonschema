sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME error
  PRIVATE_HEADERS file.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME error)
endif()
