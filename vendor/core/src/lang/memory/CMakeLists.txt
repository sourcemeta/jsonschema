sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME memory
  PRIVATE_HEADERS owned_or_reference.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME memory)
endif()
