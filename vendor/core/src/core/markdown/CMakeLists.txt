sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME markdown
  SOURCES markdown.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME markdown)
endif()

target_link_libraries(sourcemeta_core_markdown PRIVATE CMarkGFM::cmark_gfm)
