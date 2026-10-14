sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME mcp
  PRIVATE_HEADERS capabilities.h error.h headers.h logging.h protocol.h results.h
  SOURCES helpers.h validation.h parameter_headers.cc notifications.cc capabilities.cc error.cc headers.cc results.cc)

target_link_libraries(sourcemeta_core_mcp PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_mcp PUBLIC sourcemeta::core::jsonrpc)
target_link_libraries(sourcemeta_core_mcp PUBLIC sourcemeta::core::http)
target_link_libraries(sourcemeta_core_mcp PRIVATE sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_mcp PRIVATE sourcemeta::core::unicode)
target_link_libraries(sourcemeta_core_mcp PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_mcp PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_mcp PRIVATE sourcemeta::core::time)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME mcp)
endif()
