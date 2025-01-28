sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME json
  FOLDER "Core/JSON"
  PRIVATE_HEADERS array.h error.h object.h value.h hash.h
  SOURCES grammar.h parser.h stringify.h json.cc json_value.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME json)
endif()
