sourcemeta_library(NAMESPACE sourcemeta PROJECT hydra NAME http
  FOLDER "Hydra/HTTP"
  PRIVATE_HEADERS method.h status.h error.h header.h time.h mime.h
    request.h response.h stream.h
  SOURCES method.cc status.cc error.cc header.cc time.cc mime.cc
    request.cc response.cc stream_curl.cc)

if(HYDRA_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT hydra NAME http)
endif()

target_link_libraries(sourcemeta_hydra_http PRIVATE CURL::libcurl)
