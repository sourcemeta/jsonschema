sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME uri
  PRIVATE_HEADERS error.h
  SOURCES uri.cc parse.cc accessors.cc setters.cc recompose.cc canonicalize.cc
          resolution.cc filesystem.cc escaping.h normalize.h grammar.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME uri)
endif()

target_link_libraries(sourcemeta_core_uri
  PRIVATE sourcemeta::core::ip)
