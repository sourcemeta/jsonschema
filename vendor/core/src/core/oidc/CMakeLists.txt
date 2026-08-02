sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME oidc
  PRIVATE_HEADERS error.h metadata.h discovery.h hash.h id_token.h
    authentication.h claims.h userinfo.h registration.h subject.h
    request_object.h logout.h encryption.h profile.h
  SOURCES oidc_error.cc oidc_metadata.cc oidc_discovery.cc oidc_hash.cc
    oidc_id_token.cc oidc_authentication.cc oidc_claims.cc oidc_userinfo.cc
    oidc_registration.cc oidc_subject.cc oidc_verify.h
    oidc_request_object.cc oidc_logout.cc oidc_encryption.cc)

target_link_libraries(sourcemeta_core_oidc
  PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_oidc
  PUBLIC sourcemeta::core::jose)
target_link_libraries(sourcemeta_core_oidc
  PUBLIC sourcemeta::core::oauth)
target_link_libraries(sourcemeta_core_oidc
  PRIVATE sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_oidc
  PRIVATE sourcemeta::core::time)
target_link_libraries(sourcemeta_core_oidc
  PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_oidc
  PRIVATE sourcemeta::core::text)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME oidc)
endif()
