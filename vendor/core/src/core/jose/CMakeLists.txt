sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jose
  PRIVATE_HEADERS algorithm.h error.h jwk.h jwks.h
  SOURCES jose_algorithm.cc jose_jwk.cc jose_jwks.cc)

target_link_libraries(sourcemeta_core_jose
  PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jose
  PRIVATE sourcemeta::core::crypto)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jose)
endif()
