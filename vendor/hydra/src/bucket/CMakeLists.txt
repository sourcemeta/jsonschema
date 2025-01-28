sourcemeta_library(NAMESPACE sourcemeta PROJECT hydra NAME bucket
  FOLDER "Hydra/Bucket"
  PRIVATE_HEADERS cache.h cache_policy.h error.h response.h aws_sigv4.h
  SOURCES bucket.cc bucket_error.cc aws_sigv4.cc)

if(HYDRA_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT hydra NAME bucket)
endif()

target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::core::uri)
target_link_libraries(sourcemeta_hydra_bucket PRIVATE sourcemeta::hydra::crypto)
target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::hydra::http)
target_link_libraries(sourcemeta_hydra_bucket PRIVATE sourcemeta::hydra::httpclient)
