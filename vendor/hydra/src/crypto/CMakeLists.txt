noa_library(NAMESPACE sourcemeta PROJECT hydra NAME crypto
  FOLDER "Hydra/Crypto"
  SOURCES bearssl.h uuid.cc sha256.cc md5.cc base64.cc)

if(HYDRA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT hydra NAME crypto)
endif()

target_link_libraries(sourcemeta_hydra_crypto PRIVATE BearSSL::BearSSL)
