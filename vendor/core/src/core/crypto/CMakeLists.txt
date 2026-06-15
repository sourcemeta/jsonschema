sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME crypto
  PRIVATE_HEADERS sha256.h sha384.h sha512.h sha1.h fnv128.h uuid.h crc32.h
    base64.h verify.h
  SOURCES crypto_sha256.cc crypto_sha384.cc crypto_sha512.cc crypto_sha1.cc
    crypto_uuid.cc crypto_fnv128.cc crypto_crc32.cc crypto_base64.cc
    crypto_sha2_64.h crypto_random.h crypto_helpers.h)

target_link_libraries(sourcemeta_core_crypto
  PRIVATE sourcemeta::core::text sourcemeta::core::numeric)

if(SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL)
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_openssl.cc crypto_sha256_openssl.cc crypto_sha384_openssl.cc
    crypto_sha512_openssl.cc crypto_random_openssl.cc
    crypto_verify_rsa_openssl.cc crypto_verify_ecdsa_openssl.cc)
  target_link_libraries(sourcemeta_core_crypto PRIVATE OpenSSL::Crypto)
elseif(APPLE)
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_apple.cc crypto_sha256_apple.cc crypto_sha384_apple.cc
    crypto_sha512_apple.cc crypto_random_apple.cc
    crypto_verify_rsa_apple.cc crypto_verify_ecdsa_apple.cc)
  target_link_libraries(sourcemeta_core_crypto PRIVATE "-framework Security")
  target_link_libraries(sourcemeta_core_crypto
    PRIVATE "-framework CoreFoundation")
elseif(WIN32)
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_windows.cc crypto_sha256_windows.cc crypto_sha384_windows.cc
    crypto_sha512_windows.cc crypto_random_windows.cc
    crypto_verify_rsa_windows.cc crypto_verify_ecdsa_windows.cc)
  target_link_libraries(sourcemeta_core_crypto PRIVATE bcrypt)
else()
  message(WARNING "Building the reference cryptography backend, instead of the "
    "OpenSSL recommended production one")
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_other.cc crypto_sha256_other.cc crypto_sha384_other.cc
    crypto_sha512_other.cc crypto_random_other.cc
    crypto_verify_rsa_other.cc crypto_verify_ecdsa_other.cc
    crypto_bignum.h crypto_ecc.h)
endif()

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME crypto)
endif()
