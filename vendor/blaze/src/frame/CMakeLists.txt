sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME frame
  FOLDER "Blaze/Frame"
  PRIVATE_HEADERS error.h
  SOURCES frame.cc helpers.h)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME frame)
endif()

target_link_libraries(sourcemeta_blaze_frame PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_frame PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_frame PUBLIC
  sourcemeta::blaze::foundation)
target_link_libraries(sourcemeta_blaze_frame PRIVATE
  sourcemeta::core::uri)
