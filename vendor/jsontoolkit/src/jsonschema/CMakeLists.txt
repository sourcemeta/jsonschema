set(OFFICIAL_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/official_resolver.in.cc")
set(OFFICIAL_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")
include(./official_resolver.cmake)

noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonschema
  FOLDER "JSON Toolkit/JSON Schema"
  PRIVATE_HEADERS anchor.h bundle.h resolver.h walker.h reference.h
    error.h transformer.h transform_rule.h transform_bundle.h compile.h
  SOURCES jsonschema.cc default_walker.cc reference.cc anchor.cc resolver.cc
    walker.cc bundle.cc transformer.cc transform_rule.cc transform_bundle.cc
    compile.cc compile_evaluate.cc compile_json.cc compile_describe.cc
    compile_helpers.h default_compiler.cc

    default_compiler_2020_12.h
    default_compiler_2019_09.h
    default_compiler_draft7.h
    default_compiler_draft6.h
    default_compiler_draft4.h

    rules/additional_properties_default.h
    rules/const_with_type.h
    rules/content_media_type_without_encoding.h
    rules/content_schema_default.h
    rules/content_schema_without_media_type.h
    rules/else_without_if.h
    rules/enum_to_const.h
    rules/enum_with_type.h
    rules/items_array_default.h
    rules/items_schema_default.h
    rules/max_contains_without_contains.h
    rules/min_contains_without_contains.h
    rules/single_type_array.h
    rules/then_without_if.h
    rules/unevaluated_items_default.h
    rules/unevaluated_properties_default.h

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

# GCC does not allow the use of std::promise, std::future
# without compiling with pthreads support.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(sourcemeta_jsontoolkit_jsonschema PUBLIC Threads::Threads)
endif()
