noa_library(NAMESPACE sourcemeta PROJECT blaze NAME evaluator
  FOLDER "Blaze/Evaluator"
  PRIVATE_HEADERS error.h value.h template.h context.h
  SOURCES evaluator.cc context.cc trace.h)

if(BLAZE_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT blaze NAME evaluator)
endif()

if(PROJECT_IS_TOP_LEVEL)
  noa_add_vectorization_diagnostics(sourcemeta_blaze_evaluator)
endif()

target_link_libraries(sourcemeta_blaze_evaluator PUBLIC
  sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_blaze_evaluator PUBLIC
  sourcemeta::jsontoolkit::jsonpointer)
target_link_libraries(sourcemeta_blaze_evaluator PRIVATE
  sourcemeta::jsontoolkit::uri)
