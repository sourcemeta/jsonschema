sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME documentation
  FOLDER "Blaze/Documentation"
  SOURCES documentation.cc documentation_html.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME documentation)
endif()

target_link_libraries(sourcemeta_blaze_documentation PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_documentation PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::blaze::alterschema)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::core::html)
