sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME crypto
  PRIVATE_HEADERS sha256.h uuid.h
  SOURCES crypto_sha256.cc crypto_uuid.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME crypto)
endif()
