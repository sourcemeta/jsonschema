sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME editorschema
  SOURCES editorschema.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME editorschema)
endif()

target_link_libraries(sourcemeta_core_editorschema PUBLIC sourcemeta::core::jsonschema)
