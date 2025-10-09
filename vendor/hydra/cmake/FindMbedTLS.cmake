if(NOT MbedTLS_FOUND)
  set(MBEDTLS_DIR "${PROJECT_SOURCE_DIR}/vendor/mbedtls")

  set(MBEDTLS_SOURCES
    # Crypto library sources
    "${MBEDTLS_DIR}/library/aes.c"
    "${MBEDTLS_DIR}/library/aesni.c"
    "${MBEDTLS_DIR}/library/aesce.c"
    "${MBEDTLS_DIR}/library/aria.c"
    "${MBEDTLS_DIR}/library/asn1parse.c"
    "${MBEDTLS_DIR}/library/asn1write.c"
    "${MBEDTLS_DIR}/library/base64.c"
    "${MBEDTLS_DIR}/library/bignum.c"
    "${MBEDTLS_DIR}/library/bignum_core.c"
    "${MBEDTLS_DIR}/library/bignum_mod.c"
    "${MBEDTLS_DIR}/library/bignum_mod_raw.c"
    "${MBEDTLS_DIR}/library/block_cipher.c"
    "${MBEDTLS_DIR}/library/camellia.c"
    "${MBEDTLS_DIR}/library/ccm.c"
    "${MBEDTLS_DIR}/library/chacha20.c"
    "${MBEDTLS_DIR}/library/chachapoly.c"
    "${MBEDTLS_DIR}/library/cipher.c"
    "${MBEDTLS_DIR}/library/cipher_wrap.c"
    "${MBEDTLS_DIR}/library/constant_time.c"
    "${MBEDTLS_DIR}/library/cmac.c"
    "${MBEDTLS_DIR}/library/ctr_drbg.c"
    "${MBEDTLS_DIR}/library/des.c"
    "${MBEDTLS_DIR}/library/dhm.c"
    "${MBEDTLS_DIR}/library/ecdh.c"
    "${MBEDTLS_DIR}/library/ecdsa.c"
    "${MBEDTLS_DIR}/library/ecjpake.c"
    "${MBEDTLS_DIR}/library/ecp.c"
    "${MBEDTLS_DIR}/library/ecp_curves.c"
    "${MBEDTLS_DIR}/library/ecp_curves_new.c"
    "${MBEDTLS_DIR}/library/entropy.c"
    "${MBEDTLS_DIR}/library/entropy_poll.c"
    "${MBEDTLS_DIR}/library/error.c"
    "${MBEDTLS_DIR}/library/gcm.c"
    "${MBEDTLS_DIR}/library/hkdf.c"
    "${MBEDTLS_DIR}/library/hmac_drbg.c"
    "${MBEDTLS_DIR}/library/lmots.c"
    "${MBEDTLS_DIR}/library/lms.c"
    "${MBEDTLS_DIR}/library/md.c"
    "${MBEDTLS_DIR}/library/md5.c"
    "${MBEDTLS_DIR}/library/memory_buffer_alloc.c"
    "${MBEDTLS_DIR}/library/nist_kw.c"
    "${MBEDTLS_DIR}/library/oid.c"
    "${MBEDTLS_DIR}/library/padlock.c"
    "${MBEDTLS_DIR}/library/pem.c"
    "${MBEDTLS_DIR}/library/pk.c"
    "${MBEDTLS_DIR}/library/pk_ecc.c"
    "${MBEDTLS_DIR}/library/pk_wrap.c"
    "${MBEDTLS_DIR}/library/pkcs12.c"
    "${MBEDTLS_DIR}/library/pkcs5.c"
    "${MBEDTLS_DIR}/library/pkparse.c"
    "${MBEDTLS_DIR}/library/pkwrite.c"
    "${MBEDTLS_DIR}/library/platform.c"
    "${MBEDTLS_DIR}/library/platform_util.c"
    "${MBEDTLS_DIR}/library/poly1305.c"
    "${MBEDTLS_DIR}/library/psa_crypto.c"
    "${MBEDTLS_DIR}/library/psa_crypto_aead.c"
    "${MBEDTLS_DIR}/library/psa_crypto_cipher.c"
    "${MBEDTLS_DIR}/library/psa_crypto_client.c"
    "${MBEDTLS_DIR}/library/psa_crypto_driver_wrappers_no_static.c"
    "${MBEDTLS_DIR}/library/psa_crypto_ecp.c"
    "${MBEDTLS_DIR}/library/psa_crypto_ffdh.c"
    "${MBEDTLS_DIR}/library/psa_crypto_hash.c"
    "${MBEDTLS_DIR}/library/psa_crypto_mac.c"
    "${MBEDTLS_DIR}/library/psa_crypto_pake.c"
    "${MBEDTLS_DIR}/library/psa_crypto_rsa.c"
    "${MBEDTLS_DIR}/library/psa_crypto_se.c"
    "${MBEDTLS_DIR}/library/psa_crypto_slot_management.c"
    "${MBEDTLS_DIR}/library/psa_crypto_storage.c"
    "${MBEDTLS_DIR}/library/psa_its_file.c"
    "${MBEDTLS_DIR}/library/psa_util.c"
    "${MBEDTLS_DIR}/library/ripemd160.c"
    "${MBEDTLS_DIR}/library/rsa.c"
    "${MBEDTLS_DIR}/library/rsa_alt_helpers.c"
    "${MBEDTLS_DIR}/library/sha1.c"
    "${MBEDTLS_DIR}/library/sha256.c"
    "${MBEDTLS_DIR}/library/sha512.c"
    "${MBEDTLS_DIR}/library/sha3.c"
    "${MBEDTLS_DIR}/library/threading.c"
    "${MBEDTLS_DIR}/library/timing.c"
    "${MBEDTLS_DIR}/library/version.c"
    "${MBEDTLS_DIR}/library/version_features.c"
    # X509 library sources
    "${MBEDTLS_DIR}/library/pkcs7.c"
    "${MBEDTLS_DIR}/library/x509.c"
    "${MBEDTLS_DIR}/library/x509_create.c"
    "${MBEDTLS_DIR}/library/x509_crl.c"
    "${MBEDTLS_DIR}/library/x509_crt.c"
    "${MBEDTLS_DIR}/library/x509_csr.c"
    "${MBEDTLS_DIR}/library/x509write.c"
    "${MBEDTLS_DIR}/library/x509write_crt.c"
    "${MBEDTLS_DIR}/library/x509write_csr.c"
    # TLS library sources
    "${MBEDTLS_DIR}/library/debug.c"
    "${MBEDTLS_DIR}/library/mps_reader.c"
    "${MBEDTLS_DIR}/library/mps_trace.c"
    "${MBEDTLS_DIR}/library/net_sockets.c"
    "${MBEDTLS_DIR}/library/ssl_cache.c"
    "${MBEDTLS_DIR}/library/ssl_ciphersuites.c"
    "${MBEDTLS_DIR}/library/ssl_client.c"
    "${MBEDTLS_DIR}/library/ssl_cookie.c"
    "${MBEDTLS_DIR}/library/ssl_debug_helpers_generated.c"
    "${MBEDTLS_DIR}/library/ssl_msg.c"
    "${MBEDTLS_DIR}/library/ssl_ticket.c"
    "${MBEDTLS_DIR}/library/ssl_tls.c"
    "${MBEDTLS_DIR}/library/ssl_tls12_client.c"
    "${MBEDTLS_DIR}/library/ssl_tls12_server.c"
    "${MBEDTLS_DIR}/library/ssl_tls13_keys.c"
    "${MBEDTLS_DIR}/library/ssl_tls13_server.c"
    "${MBEDTLS_DIR}/library/ssl_tls13_client.c"
    "${MBEDTLS_DIR}/library/ssl_tls13_generic.c")

  if(WIN32)
    add_library(mbedtls STATIC ${MBEDTLS_SOURCES})
  else()
    add_library(mbedtls ${MBEDTLS_SOURCES})
  endif()

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(mbedtls PRIVATE
      /W3 /MP /wd4244 /wd4267 /wd4996 /wd4146)
    target_compile_definitions(mbedtls PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(mbedtls PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wdouble-promotion
      -Wfloat-equal
      -Wformat=2
      -Wmissing-declarations
      -Wwrite-strings
      -Wno-cast-align
      -Wno-cast-qual
      -Wno-unused-parameter
      -Wno-sign-conversion
      -Wno-implicit-int-conversion
      -Wno-sign-compare
      -Wno-shadow
      -Wno-conditional-uninitialized
      -Wno-documentation-deprecated-sync
      -Wno-strict-overflow)

    if(HYDRA_COMPILER_GCC)
      target_compile_options(mbedtls PRIVATE -Wno-stringop-overflow)
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(mbedtls PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()
  endif()

  target_include_directories(mbedtls PRIVATE "${MBEDTLS_DIR}/library")
  target_include_directories(mbedtls PUBLIC
    "$<BUILD_INTERFACE:${MBEDTLS_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(MbedTLS::mbedtls ALIAS mbedtls)

  set_target_properties(mbedtls
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME mbedtls
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME mbedtls)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS mbedtls
      EXPORT mbedtls
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT mbedtls
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mbedtls"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/mbedtls-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/mbedtls.cmake\")\n"
      "check_required_components(\"mbedtls\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/mbedtls-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mbedtls"
      COMPONENT sourcemeta_hydra_dev)

    install(DIRECTORY "${MBEDTLS_DIR}/include/mbedtls"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
    install(DIRECTORY "${MBEDTLS_DIR}/include/psa"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(MbedTLS_FOUND ON)
endif()
