noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME yaml
  FOLDER "JSON Toolkit/YAML"
  PRIVATE_HEADERS error.h
  SOURCES yaml.cc)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME yaml)
endif()

target_link_libraries(sourcemeta_jsontoolkit_yaml PRIVATE yaml)
target_link_libraries(sourcemeta_jsontoolkit_yaml PUBLIC sourcemeta::jsontoolkit::json)
