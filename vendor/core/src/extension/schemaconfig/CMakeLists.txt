sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME schemaconfig
  PRIVATE_HEADERS error.h SOURCES parse.cc find.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME schemaconfig)
endif()

target_link_libraries(sourcemeta_core_schemaconfig PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_schemaconfig PUBLIC sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_schemaconfig PRIVATE sourcemeta::core::uri)
