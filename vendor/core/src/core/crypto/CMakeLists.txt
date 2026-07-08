sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME crypto
  PRIVATE_HEADERS sha256.h hmac_sha256.h hmac_sha384.h hmac_sha512.h sha384.h
    sha512.h sha1.h
    fnv128.h
    uuid.h crc32.h base64.h verify.h sign.h aes_gcm.h
  SOURCES crypto_sha256.cc crypto_hmac_sha256.cc crypto_hmac_sha384.cc
    crypto_hmac_sha512.cc
    crypto_sha384.cc
    crypto_sha512.cc crypto_sha1.cc
    crypto_uuid.cc crypto_fnv128.cc crypto_crc32.cc crypto_base64.cc
    crypto_random.cc crypto_sign_rsa.cc crypto_aes_gcm.cc
    crypto_sha2_64.h crypto_random.h crypto_helpers.h crypto_der.h crypto_aes.h)

target_link_libraries(sourcemeta_core_crypto
  PRIVATE sourcemeta::core::text sourcemeta::core::numeric)

if(SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL)
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_openssl.cc crypto_sha256_openssl.cc
    crypto_hmac_sha256_openssl.cc crypto_hmac_sha384_openssl.cc
    crypto_hmac_sha512_openssl.cc
    crypto_sha384_openssl.cc
    crypto_sha512_openssl.cc crypto_random_openssl.cc
    crypto_verify_openssl.cc crypto_secure_equals_openssl.cc
    crypto_sign_openssl.cc crypto_aes_gcm_openssl.cc)
  target_link_libraries(sourcemeta_core_crypto PRIVATE OpenSSL::Crypto)
elseif(APPLE)
  enable_language(OBJCXX)

  # Ed25519 is verified through CryptoKit (Swift), reached from Objective-C++.
  # The Swift shim is compiled to an object and its generated Objective-C
  # interface header out of band, since the rest of the library is C++
  set(CRYPTOKIT_SWIFT "${CMAKE_CURRENT_SOURCE_DIR}/crypto_eddsa_cryptokit.swift")
  set(CRYPTOKIT_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/sourcemeta_core_cryptokit-Swift.h")
  if(CMAKE_OSX_SYSROOT)
    set(CRYPTOKIT_SDK "${CMAKE_OSX_SYSROOT}")
  else()
    execute_process(COMMAND xcrun --show-sdk-path
      OUTPUT_VARIABLE CRYPTOKIT_SDK OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  # The toolchain directory holding the Swift runtime back-deployment archives
  # that the shim autolinks for older deployment targets
  execute_process(COMMAND xcrun --find swiftc
    OUTPUT_VARIABLE CRYPTOKIT_SWIFTC OUTPUT_STRIP_TRAILING_WHITESPACE)
  get_filename_component(CRYPTOKIT_TOOLCHAIN_BIN "${CRYPTOKIT_SWIFTC}" DIRECTORY)
  get_filename_component(CRYPTOKIT_RUNTIME_LIB
    "${CRYPTOKIT_TOOLCHAIN_BIN}/../lib/swift/macosx" ABSOLUTE)
  if(CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CRYPTOKIT_DEPLOYMENT "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  else()
    # CryptoKit signing over Curve25519 is available since macOS 10.15
    set(CRYPTOKIT_DEPLOYMENT "10.15")
  endif()
  if(CMAKE_OSX_ARCHITECTURES)
    set(CRYPTOKIT_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
  else()
    set(CRYPTOKIT_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  # One object per architecture, emitting the architecture independent header
  # only on the first, combined afterwards into a single object
  set(CRYPTOKIT_OBJECTS)
  set(CRYPTOKIT_HEADER_OUTPUT "${CRYPTOKIT_HEADER}")
  set(CRYPTOKIT_EMIT_HEADER
    -emit-objc-header -emit-objc-header-path "${CRYPTOKIT_HEADER}")
  foreach(architecture IN LISTS CRYPTOKIT_ARCHITECTURES)
    set(CRYPTOKIT_ARCH_OBJECT
      "${CMAKE_CURRENT_BINARY_DIR}/crypto_eddsa_cryptokit_${architecture}.o")
    add_custom_command(
      OUTPUT "${CRYPTOKIT_ARCH_OBJECT}" ${CRYPTOKIT_HEADER_OUTPUT}
      COMMAND xcrun swiftc
        -sdk "${CRYPTOKIT_SDK}"
        -target "${architecture}-apple-macosx${CRYPTOKIT_DEPLOYMENT}"
        -module-name sourcemeta_core_cryptokit
        -parse-as-library -O
        ${CRYPTOKIT_EMIT_HEADER}
        -emit-object -o "${CRYPTOKIT_ARCH_OBJECT}"
        "${CRYPTOKIT_SWIFT}"
      DEPENDS "${CRYPTOKIT_SWIFT}"
      COMMENT "Building CryptoKit Swift shim (${architecture})"
      VERBATIM)
    list(APPEND CRYPTOKIT_OBJECTS "${CRYPTOKIT_ARCH_OBJECT}")
    set(CRYPTOKIT_HEADER_OUTPUT)
    set(CRYPTOKIT_EMIT_HEADER)
  endforeach()

  list(LENGTH CRYPTOKIT_OBJECTS CRYPTOKIT_OBJECT_COUNT)
  if(CRYPTOKIT_OBJECT_COUNT GREATER 1)
    set(CRYPTOKIT_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/crypto_eddsa_cryptokit.o")
    add_custom_command(
      OUTPUT "${CRYPTOKIT_OBJECT}"
      COMMAND lipo -create ${CRYPTOKIT_OBJECTS} -output "${CRYPTOKIT_OBJECT}"
      DEPENDS ${CRYPTOKIT_OBJECTS}
      COMMENT "Combining CryptoKit Swift shim architectures"
      VERBATIM)
  else()
    set(CRYPTOKIT_OBJECT "${CRYPTOKIT_OBJECTS}")
  endif()

  set_source_files_properties("${CRYPTOKIT_OBJECT}"
    PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  set_source_files_properties("${CRYPTOKIT_HEADER}" PROPERTIES GENERATED TRUE)
  set_source_files_properties(crypto_eddsa_cryptokit.mm
    PROPERTIES OBJECT_DEPENDS "${CRYPTOKIT_HEADER}")

  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_apple.cc crypto_sha256_apple.cc crypto_hmac_sha256_apple.cc
    crypto_hmac_sha384_apple.cc crypto_hmac_sha512_apple.cc
    crypto_sha384_apple.cc
    crypto_sha512_apple.cc crypto_random_apple.cc
    crypto_verify_apple.cc crypto_secure_equals_apple.cc
    crypto_sign_apple.cc crypto_aes_gcm_apple.cc
    crypto_eddsa_cryptokit.mm "${CRYPTOKIT_OBJECT}"
    crypto_eddsa.h crypto_eddsa_apple.h crypto_aes_apple.h crypto_bignum.h
    crypto_shake256.h crypto_pkcs8.h)

  # The generated Objective-C interface header lives in the build tree
  target_include_directories(sourcemeta_core_crypto
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

  target_link_libraries(sourcemeta_core_crypto PRIVATE "-framework Security")
  target_link_libraries(sourcemeta_core_crypto
    PRIVATE "-framework CoreFoundation")
  target_link_libraries(sourcemeta_core_crypto PRIVATE "-framework CryptoKit")
  target_link_libraries(sourcemeta_core_crypto PRIVATE "-framework Foundation")

  # Resolve the Swift runtime that the shim autolinks, both at link and at load.
  # PUBLIC rather than INTERFACE so that a shared build of this library, which
  # has its own link step pulling in the Swift object, also gets the flags
  target_link_options(sourcemeta_core_crypto PUBLIC
    "SHELL:-L ${CRYPTOKIT_SDK}/usr/lib/swift"
    "SHELL:-L ${CRYPTOKIT_RUNTIME_LIB}"
    "SHELL:-Xlinker -rpath -Xlinker /usr/lib/swift")
elseif(WIN32)
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_windows.cc crypto_sha256_windows.cc
    crypto_hmac_sha256_windows.cc crypto_hmac_sha384_windows.cc
    crypto_hmac_sha512_windows.cc
    crypto_sha384_windows.cc
    crypto_sha512_windows.cc crypto_random_windows.cc
    crypto_verify_windows.cc crypto_secure_equals_generic.cc
    crypto_sign_windows.cc crypto_aes_gcm_windows.cc crypto_eddsa.h
    crypto_bignum.h crypto_ecc.h crypto_shake256.h crypto_pkcs8.h)
  target_link_libraries(sourcemeta_core_crypto PRIVATE bcrypt)
else()
  message(WARNING "Building the reference cryptography backend, instead of the "
    "OpenSSL recommended production one")
  target_sources(sourcemeta_core_crypto PRIVATE
    crypto_sha1_other.cc crypto_sha256_other.cc crypto_hmac_sha256_other.cc
    crypto_hmac_sha384_other.cc crypto_hmac_sha512_other.cc
    crypto_sha384_other.cc
    crypto_sha512_other.cc crypto_random_other.cc crypto_verify_other.cc
    crypto_secure_equals_generic.cc crypto_sign_other.cc
    crypto_aes_gcm_other.cc
    crypto_bignum.h crypto_ecc.h crypto_eddsa.h crypto_shake256.h
    crypto_pkcs8.h)
endif()

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME crypto)
endif()
