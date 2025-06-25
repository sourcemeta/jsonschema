sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME json
  PRIVATE_HEADERS array.h error.h object.h value.h hash.h auto.h
  SOURCES grammar.h parser.h stringify.h json.cc json_value.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME json)
endif()
