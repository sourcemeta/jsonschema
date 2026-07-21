sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonld
  PRIVATE_HEADERS error.h materialize.h
  SOURCES jsonld.cc
    jsonld_iri_expansion.cc jsonld_create_term_definition.cc
    jsonld_context_processing.cc jsonld_value_expansion.cc jsonld_expansion.cc
    jsonld_is_expanded.cc
    jsonld_inverse_context.cc jsonld_iri_compaction.cc
    jsonld_value_compaction.cc jsonld_compaction.cc
    jsonld_node_map.cc jsonld_flatten.cc
    jsonld_materialize.cc
    jsonld_algorithms.h jsonld_keywords.h jsonld_serialise.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonld)
endif()

target_link_libraries(sourcemeta_core_jsonld
  PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jsonld
  PUBLIC sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_jsonld
  PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_jsonld
  PRIVATE sourcemeta::core::langtag)
target_link_libraries(sourcemeta_core_jsonld
  PRIVATE sourcemeta::core::text)
