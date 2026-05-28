sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME mcp
  SOURCES mcp.cc)

target_link_libraries(sourcemeta_core_mcp PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_mcp PUBLIC sourcemeta::core::jsonrpc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME mcp)
endif()
