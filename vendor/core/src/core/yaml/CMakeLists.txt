sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME yaml
  FOLDER "Core/YAML"
  PRIVATE_HEADERS error.h
  SOURCES yaml.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME yaml)
endif()

target_link_libraries(sourcemeta_core_yaml PRIVATE yaml)
target_link_libraries(sourcemeta_core_yaml PUBLIC sourcemeta::core::json)
