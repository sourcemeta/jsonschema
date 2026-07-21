sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME oauth
  PRIVATE_HEADERS error.h profile.h pkce.h bearer.h random.h authorization.h
    token.h client_authentication.h transaction.h metadata.h metadata_provider.h
    token_exchange.h revocation.h introspection.h device.h dpop.h par.h
    assertion.h registration.h
  SOURCES oauth_error.cc oauth_pkce.cc oauth_bearer.cc oauth_syntax.h
    oauth_random.cc oauth_authorization.cc oauth_authorization_parse.h
    oauth_encode.h oauth_decode.h
    oauth_json.h oauth_token.cc oauth_client_authentication.cc
    oauth_transaction.cc oauth_metadata.cc oauth_metadata_provider.cc
    oauth_ttl.h oauth_token_exchange.cc oauth_revocation.cc
    oauth_introspection.cc oauth_device.cc oauth_dpop.cc oauth_par.cc
    oauth_assertion.cc oauth_registration.cc)

target_link_libraries(sourcemeta_core_oauth
  PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_oauth
  PUBLIC sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_oauth
  PUBLIC sourcemeta::core::jose)
target_link_libraries(sourcemeta_core_oauth
  PUBLIC sourcemeta::core::http)
target_link_libraries(sourcemeta_core_oauth
  PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_oauth
  PRIVATE sourcemeta::core::text)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME oauth)
endif()
