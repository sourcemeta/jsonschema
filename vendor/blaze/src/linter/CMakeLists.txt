sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME linter
  FOLDER "Blaze/Linter"
  SOURCES
    valid_default.cc
    valid_examples.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME linter)
endif()

target_link_libraries(sourcemeta_blaze_linter PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_linter PUBLIC
  sourcemeta::blaze::compiler)
target_link_libraries(sourcemeta_blaze_linter PRIVATE
  sourcemeta::blaze::evaluator)
target_link_libraries(sourcemeta_blaze_linter PRIVATE
  sourcemeta::blaze::output)
