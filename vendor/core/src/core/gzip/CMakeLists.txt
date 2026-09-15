sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME gzip
  PRIVATE_HEADERS error.h streambuf.h
  SOURCES gzip.cc streambuf.cc)

target_link_libraries(sourcemeta_core_gzip PRIVATE sourcemeta::core::crypto)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME gzip)
endif()
