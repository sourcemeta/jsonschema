sourcemeta_library(NAMESPACE sourcemeta PROJECT jsonschema NAME http
  FOLDER "JSON Schema/HTTP"
  PRIVATE_HEADERS method.h status.h error.h request.h response.h stream.h
  SOURCES status.cc error.cc request.cc response.cc stream_curl.cc)

target_link_libraries(sourcemeta_jsonschema_http PRIVATE CURL::libcurl)
