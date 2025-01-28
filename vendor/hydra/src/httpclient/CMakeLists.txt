sourcemeta_library(NAMESPACE sourcemeta PROJECT hydra NAME httpclient
  FOLDER "Hydra/HTTP Client"
  PRIVATE_HEADERS stream.h request.h response.h
  SOURCES stream_curl.cc request.cc response.cc)

if(HYDRA_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT hydra NAME httpclient)
endif()

target_link_libraries(sourcemeta_hydra_httpclient PRIVATE CURL::libcurl)
target_link_libraries(sourcemeta_hydra_httpclient PUBLIC sourcemeta::hydra::http)
