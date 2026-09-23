sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME configuration
  PRIVATE_HEADERS error.h SOURCES parse.cc json.cc configuration.cc lock.cc fetch.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME configuration)
endif()

target_link_libraries(sourcemeta_blaze_configuration PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_configuration PUBLIC sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_configuration PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_blaze_configuration PRIVATE sourcemeta::core::io)
target_link_libraries(sourcemeta_blaze_configuration PRIVATE sourcemeta::core::crypto)
target_link_libraries(sourcemeta_blaze_configuration PRIVATE sourcemeta::core::jsonschema)
