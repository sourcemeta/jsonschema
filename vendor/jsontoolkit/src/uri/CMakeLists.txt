noa_library(NAMESPACE sourcemeta PROJECT jsontoolkit NAME uri
  FOLDER "JSON Toolkit/URI"
  PRIVATE_HEADERS error.h
  SOURCES uri.cc escaping.cc)

if(JSONTOOLKIT_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsontoolkit NAME uri)
endif()

target_link_libraries(sourcemeta_jsontoolkit_uri
  PRIVATE uriparser::uriparser)
