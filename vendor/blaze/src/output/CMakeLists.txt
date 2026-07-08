sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME output
  FOLDER "Blaze/Output"
  PRIVATE_HEADERS simple.h trace.h standard.h jsonld.h
  SOURCES
    output_simple.cc
    output_trace.cc
    output_standard.cc
    output_jsonld.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME output)
endif()

target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_output PRIVATE
  sourcemeta::core::jsonld)
target_link_libraries(sourcemeta_blaze_output PRIVATE
  sourcemeta::core::langtag)
target_link_libraries(sourcemeta_blaze_output PRIVATE
  sourcemeta::core::text)
target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::blaze::foundation)
target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::blaze::frame)

target_link_libraries(sourcemeta_blaze_output PUBLIC
  sourcemeta::blaze::evaluator)
