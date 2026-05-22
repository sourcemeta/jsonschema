sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME editor
  FOLDER "Blaze/Editor"
  SOURCES editor.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME editor)
endif()

target_link_libraries(sourcemeta_blaze_editor PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_editor PUBLIC
  sourcemeta::blaze::foundation)
target_link_libraries(sourcemeta_blaze_editor PRIVATE
  sourcemeta::blaze::frame)
