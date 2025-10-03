sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME gzip
  PRIVATE_HEADERS error.h tar.h
  SOURCES gzip.cc tar.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME gzip)
endif()

target_link_libraries(sourcemeta_core_gzip PRIVATE ZLIB::ZLIB)
target_link_libraries(sourcemeta_core_gzip PRIVATE Archive::Archive)
