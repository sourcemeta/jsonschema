sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME gzip
  PRIVATE_HEADERS error.h
  SOURCES gzip.cc)

target_link_libraries(sourcemeta_core_gzip PRIVATE LibDeflate::LibDeflate)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME gzip)
endif()
