sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME dependencies
  FOLDER "Blaze/Dependencies"
  SOURCES dependencies.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME dependencies)
endif()

target_link_libraries(sourcemeta_blaze_dependencies PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_dependencies PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_dependencies PUBLIC
  sourcemeta::core::jsonschema)
