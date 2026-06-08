sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME crypto
  PRIVATE_HEADERS sha256.h uuid.h crc32.h
  SOURCES crypto_sha256.cc crypto_uuid.cc crypto_crc32.cc)

if(SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL)
  target_compile_definitions(sourcemeta_core_crypto
    PRIVATE SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL)
  target_link_libraries(sourcemeta_core_crypto PRIVATE OpenSSL::Crypto)
endif()

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME crypto)
endif()
