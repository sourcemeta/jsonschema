noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonl
  FOLDER "JSON Toolkit/JSONL"
  PRIVATE_HEADERS iterator.h
  SOURCES jsonl.cc iterator.cc grammar.h)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME jsonl)
endif()

target_link_libraries(sourcemeta_jsontoolkit_jsonl PUBLIC
  sourcemeta::jsontoolkit::json)
