sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME bundle
  FOLDER "Blaze/Bundle"
  SOURCES bundle.cc helpers.h)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME bundle)
endif()

target_link_libraries(sourcemeta_blaze_bundle PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_bundle PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_bundle PUBLIC
  sourcemeta::blaze::foundation)
target_link_libraries(sourcemeta_blaze_bundle PUBLIC
  sourcemeta::blaze::frame)
target_link_libraries(sourcemeta_blaze_bundle PRIVATE
  sourcemeta::core::uri)
