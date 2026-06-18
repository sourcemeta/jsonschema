sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jose
  PRIVATE_HEADERS algorithm.h error.h jwk.h jwks.h jwt.h verify.h
  SOURCES jose_algorithm.cc jose_jwk.cc jose_jwks.cc jose_jwt.cc
          jose_jwt_check_claims.cc jose_jws_verify_signature.cc
          jose_jwt_verify_signature.cc jose_jwt_verify.cc)

target_link_libraries(sourcemeta_core_jose
  PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jose
  PUBLIC sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_jose
  PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_jose
  PRIVATE sourcemeta::core::time)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jose)
endif()
