set(OFFICIAL_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/official_resolver.in.cc")
set(OFFICIAL_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/official_resolver.cc")
include(./official_resolver.cmake)

noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonschema
  FOLDER "JSON Toolkit/JSON Schema"
  PRIVATE_HEADERS anchor.h bundle.h resolver.h
    walker.h reference.h error.h compile.h
  SOURCES jsonschema.cc default_walker.cc reference.cc
    anchor.cc resolver.cc
    walker.cc bundle.cc compile.cc
    compile_json.cc compile_describe.cc
    compile_helpers.h default_compiler.cc

    default_compiler_2020_12.h
    default_compiler_2019_09.h
    default_compiler_draft7.h
    default_compiler_draft6.h
    default_compiler_draft4.h

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
target_link_libraries(sourcemeta_jsontoolkit_jsonschema PUBLIC
  sourcemeta::jsontoolkit::evaluator)

# GCC does not allow the use of std::promise, std::future
# without compiling with pthreads support.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  target_link_libraries(sourcemeta_jsontoolkit_jsonschema PUBLIC Threads::Threads)
endif()
