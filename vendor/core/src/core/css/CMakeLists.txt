sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME css
  SOURCES css2_color.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME css)
endif()

target_link_libraries(sourcemeta_core_css PRIVATE sourcemeta::core::numeric)
target_link_libraries(sourcemeta_core_css PRIVATE sourcemeta::core::text)
