sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME markdown
  PRIVATE_HEADERS error.h
  SOURCES markdown.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME markdown)
endif()

target_link_libraries(sourcemeta_core_markdown PRIVATE
  sourcemeta::core::email
  sourcemeta::core::html
  sourcemeta::core::text
  sourcemeta::core::unicode
  sourcemeta::core::uri)
