sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME evaluator
  FOLDER "Blaze/Evaluator"
  PRIVATE_HEADERS error.h value.h instruction.h string_set.h
  SOURCES evaluator.cc dispatch.inc.h
          evaluator_string_set.cc
          evaluator_complete.h evaluator_dynamic.h
          evaluator_track.h evaluator_fast.h)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME evaluator)
endif()

if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_add_vectorization_diagnostics(sourcemeta_blaze_evaluator)
endif()

target_link_libraries(sourcemeta_blaze_evaluator PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_evaluator PUBLIC
  sourcemeta::core::regex)
target_link_libraries(sourcemeta_blaze_evaluator PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_evaluator PRIVATE
  sourcemeta::core::uri)
