sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME uri
  PRIVATE_HEADERS escape.h error.h
  SOURCES uri.cc escaping.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME uri)
endif()

target_link_libraries(sourcemeta_core_uri
  PRIVATE uriparser::uriparser)
