set(KNOWN_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/known_resolver.in.cc")
set(KNOWN_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/known_resolver.cc")
include(./known_resolver.cmake)

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonschema
  PRIVATE_HEADERS bundle.h walker.h frame.h error.h
                  types.h transform.h vocabularies.h
  SOURCES jsonschema.cc vocabularies.cc known_walker.cc
          frame.cc walker.cc bundle.cc transformer.cc format.cc helpers.h
    "${CMAKE_CURRENT_BINARY_DIR}/known_resolver.cc")

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonschema)
endif()

target_link_libraries(sourcemeta_core_jsonschema PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jsonschema PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_jsonschema PRIVATE
  sourcemeta::core::uri)
