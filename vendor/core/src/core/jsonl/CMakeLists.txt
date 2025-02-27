sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonl
  PRIVATE_HEADERS iterator.h
  SOURCES jsonl.cc iterator.cc grammar.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonl)
endif()

target_link_libraries(sourcemeta_core_jsonl PUBLIC
  sourcemeta::core::json)
