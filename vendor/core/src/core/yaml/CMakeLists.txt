sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME yaml
  PRIVATE_HEADERS error.h
  SOURCES yaml.cc lexer.h parser.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME yaml)
endif()

target_link_libraries(sourcemeta_core_yaml PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_yaml PRIVATE sourcemeta::core::io)
target_link_libraries(sourcemeta_core_yaml PRIVATE sourcemeta::core::unicode)
