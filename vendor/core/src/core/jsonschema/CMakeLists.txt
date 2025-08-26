set(OFFICIAL_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/official_resolver.in.cc")
set(OFFICIAL_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")
include(./official_resolver.cmake)

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonschema
  PRIVATE_HEADERS bundle.h resolver.h walker.h frame.h error.h
                  types.h transform.h
  SOURCES jsonschema.cc official_walker.cc frame.cc resolver.cc
          walker.cc bundle.cc transformer.cc
    "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonschema)
endif()

target_link_libraries(sourcemeta_core_jsonschema PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jsonschema PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_jsonschema PRIVATE
  sourcemeta::core::uri)
