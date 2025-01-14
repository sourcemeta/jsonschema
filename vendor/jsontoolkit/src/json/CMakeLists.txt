noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME json
  FOLDER "JSON Toolkit/JSON"
  PRIVATE_HEADERS array.h error.h object.h value.h hash.h flat_map.h
  SOURCES grammar.h parser.h stringify.h json.cc json_value.cc)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME json)
endif()
