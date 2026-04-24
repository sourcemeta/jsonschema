sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME gzip
  PRIVATE_HEADERS error.h streambuf.h
  SOURCES gzip.cc streambuf.cc)

# Way faster for full buffer decompression
target_link_libraries(sourcemeta_core_gzip PRIVATE LibDeflate::LibDeflate)
# Supports streaming
target_link_libraries(sourcemeta_core_gzip PRIVATE ZLIB::ZLIB)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME gzip)
endif()
