sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME preprocessor)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME preprocessor)
endif()
