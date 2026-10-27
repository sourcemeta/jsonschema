sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME metapack
  PRIVATE_HEADERS error.h format.h read.h write.h
  SOURCES metapack_read.cc metapack_write.cc)

target_link_libraries(sourcemeta_core_metapack PUBLIC sourcemeta::core::io)
target_link_libraries(sourcemeta_core_metapack PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_metapack PRIVATE sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_metapack PRIVATE sourcemeta::core::gzip)
target_link_libraries(sourcemeta_core_metapack PRIVATE sourcemeta::core::text)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME metapack)
endif()
