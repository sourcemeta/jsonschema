sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME format
  FOLDER "Blaze/Format"
  SOURCES format.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME format)
endif()

target_link_libraries(sourcemeta_blaze_format PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_format PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_format PUBLIC
  sourcemeta::blaze::foundation)
target_link_libraries(sourcemeta_blaze_format PRIVATE
  sourcemeta::blaze::frame)
