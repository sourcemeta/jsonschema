sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME html
  PRIVATE_HEADERS buffer.h escape.h writer.h
  SOURCES escape.cc writer.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME html)
endif()

target_link_libraries(sourcemeta_core_html PUBLIC sourcemeta::core::preprocessor)
