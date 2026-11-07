sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME documentation
  FOLDER "Blaze/Documentation"
  PRIVATE_HEADERS error.h
  SOURCES documentation.cc documentation_value.cc documentation_reference.cc
    documentation_reach.cc documentation_object.cc documentation_facts.cc
    documentation_html.cc documentation_html_words.cc
    documentation_html_table.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME documentation)
endif()

target_link_libraries(sourcemeta_blaze_documentation PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_documentation PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_documentation PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::core::regex)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::core::html)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::core::text)
target_link_libraries(sourcemeta_blaze_documentation PRIVATE
  sourcemeta::core::unicode)
