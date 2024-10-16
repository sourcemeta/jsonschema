noa_library(NAMESPACE sourcemeta PROJECT hydra NAME httpserver
  FOLDER "Hydra/HTTP Server"
  PRIVATE_HEADERS request.h response.h logger.h
  SOURCES uwebsockets.h httpserver.cc request.cc response.cc logger.cc static.cc)

if(HYDRA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT hydra NAME httpserver)
endif()

target_link_libraries(sourcemeta_hydra_httpserver PRIVATE sourcemeta::hydra::crypto)
target_link_libraries(sourcemeta_hydra_httpserver PUBLIC sourcemeta::hydra::http)
target_link_libraries(sourcemeta_hydra_httpserver PUBLIC sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_hydra_httpserver PRIVATE ZLIB::ZLIB)
target_link_libraries(sourcemeta_hydra_httpserver PRIVATE uNetworking::uSockets)
target_link_libraries(sourcemeta_hydra_httpserver PRIVATE uNetworking::uWebSockets)
