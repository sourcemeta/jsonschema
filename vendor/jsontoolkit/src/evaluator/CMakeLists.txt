noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME evaluator
  FOLDER "JSON Toolkit/Evaluator"
  PRIVATE_HEADERS error.h value.h template.h context.h
  SOURCES evaluator.cc context.cc trace.h)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME evaluator)
endif()

if(PROJECT_IS_TOP_LEVEL)
  noa_add_vectorization_diagnostics(sourcemeta_jsontoolkit_evaluator)
endif()

target_link_libraries(sourcemeta_jsontoolkit_evaluator PUBLIC
  sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_jsontoolkit_evaluator PUBLIC
  sourcemeta::jsontoolkit::jsonpointer)
target_link_libraries(sourcemeta_jsontoolkit_evaluator PRIVATE
  sourcemeta::jsontoolkit::uri)
