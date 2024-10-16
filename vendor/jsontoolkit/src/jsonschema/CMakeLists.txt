set(OFFICIAL_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/official_resolver.in.cc")
set(OFFICIAL_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")
include(./official_resolver.cmake)

noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonschema
  FOLDER "JSON Toolkit/JSON Schema"
  PRIVATE_HEADERS anchor.h bundle.h resolver.h
    walker.h reference.h error.h
  SOURCES jsonschema.cc default_walker.cc reference.cc
    anchor.cc resolver.cc
    walker.cc bundle.cc
    "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonschema)
endif()

target_link_libraries(sourcemeta_jsontoolkit_jsonschema PUBLIC
  sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_jsontoolkit_jsonschema PUBLIC
  sourcemeta::jsontoolkit::jsonpointer)
target_link_libraries(sourcemeta_jsontoolkit_jsonschema PRIVATE
  sourcemeta::jsontoolkit::uri)
