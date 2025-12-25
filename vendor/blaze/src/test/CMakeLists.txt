sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME test
  FOLDER "Blaze/Test"
  PRIVATE_HEADERS error.h
  SOURCES test_parser.cc test_runner.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME test)
endif()

target_link_libraries(sourcemeta_blaze_test PRIVATE
  sourcemeta::core::uri)
target_link_libraries(sourcemeta_blaze_test PRIVATE
  sourcemeta::core::io)
target_link_libraries(sourcemeta_blaze_test PRIVATE
  sourcemeta::core::yaml)
target_link_libraries(sourcemeta_blaze_test PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_test PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_test PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_test PUBLIC
  sourcemeta::blaze::compiler)
target_link_libraries(sourcemeta_blaze_test PUBLIC
  sourcemeta::blaze::evaluator)
