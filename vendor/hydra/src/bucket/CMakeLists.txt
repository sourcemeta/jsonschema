noa_library(NAMESPACE sourcemeta PROJECT hydra NAME bucket
  FOLDER "Hydra/Bucket"
  PRIVATE_HEADERS cache.h cache_policy.h error.h response.h aws_sigv4.h
  SOURCES bucket.cc bucket_error.cc aws_sigv4.cc)

if(HYDRA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT hydra NAME bucket)
endif()

target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::jsontoolkit::uri)
target_link_libraries(sourcemeta_hydra_bucket PUBLIC sourcemeta::hydra::http)
target_link_libraries(sourcemeta_hydra_bucket PRIVATE sourcemeta::hydra::httpclient)
target_link_libraries(sourcemeta_hydra_bucket PRIVATE BearSSL::BearSSL)
sourcemeta_hydra_add_compile_options(sourcemeta_hydra_bucket)
