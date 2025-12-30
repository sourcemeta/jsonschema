sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonpointer
  PRIVATE_HEADERS pointer.h position.h error.h token.h
    walker.h
  SOURCES jsonpointer.cc stringify.h parser.h grammar.h position.cc mangle.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonpointer)
endif()

target_link_libraries(sourcemeta_core_jsonpointer PUBLIC
  sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_jsonpointer PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jsonpointer PUBLIC
  sourcemeta::core::regex)
