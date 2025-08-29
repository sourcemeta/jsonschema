sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME output
  FOLDER "Blaze/Output"
  PRIVATE_HEADERS simple.h trace.h standard.h
  SOURCES
    output_simple.cc
    output_trace.cc
    output_standard.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME output)
endif()

target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::core::jsonschema)

target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::blaze::evaluator)
