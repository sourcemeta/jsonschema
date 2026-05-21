sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonrpc
  SOURCES jsonrpc.cc)

target_link_libraries(sourcemeta_core_jsonrpc PUBLIC sourcemeta::core::json)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonrpc)
endif()
