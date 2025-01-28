noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME json
  FOLDER "JSON Toolkit/JSON"
  PRIVATE_HEADERS array.h error.h object.h value.h hash.h 
  SOURCES grammar.h parser.h stringify.h json.cc json_value.cc)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME json)
endif()

target_link_libraries(sourcemeta_jsontoolkit_json 
  PUBLIC sourcemeta::noa::flat_map)
