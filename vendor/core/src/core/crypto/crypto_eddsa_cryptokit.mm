#include "crypto_aes_apple.h"
#include "crypto_eddsa_apple.h"

#include <cstring> // std::memcpy

#import <Foundation/Foundation.h> // NSData

// The Objective-C interface generated from the Swift shim
#import "sourcemeta_core_cryptokit-Swift.h"

extern "C" auto sourcemeta_core_eddsa_ed25519_verify_cryptokit(
    const unsigned char *public_key, std::size_t public_key_size,
    const unsigned char *message, std::size_t message_size,
    const unsigned char *signature, std::size_t signature_size) -> bool {
  @autoreleasepool {
    NSData *const key{[NSData dataWithBytes:public_key length:public_key_size]};
    NSData *const payload{[NSData dataWithBytes:message length:message_size]};
    NSData *const tag{[NSData dataWithBytes:signature length:signature_size]};
    return [SourcemetaCoreEd25519 verifyWithPublicKey:key
                                              message:payload
                                            signature:tag] == YES;
  }
}

extern "C" auto sourcemeta_core_eddsa_ed25519_sign_cryptokit(
    const unsigned char *seed, std::size_t seed_size,
    const unsigned char *message, std::size_t message_size,
    unsigned char *signature) -> bool {
  @autoreleasepool {
    NSData *const key{[NSData dataWithBytes:seed length:seed_size]};
    NSData *const payload{[NSData dataWithBytes:message length:message_size]};
    NSData *const result{[SourcemetaCoreEd25519 signWithSeed:key
                                                     message:payload]};
    if (signature == nullptr || result == nil || result.length != 64) {
      return false;
    }

    std::memcpy(signature, result.bytes, 64);
    return true;
  }
}

extern "C" auto sourcemeta_core_aes_256_gcm_seal_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *plaintext,
    std::size_t plaintext_size, unsigned char *output) -> bool {
  @autoreleasepool {
    NSData *const key_data{[NSData dataWithBytes:key length:key_size]};
    NSData *const nonce_data{[NSData dataWithBytes:nonce length:nonce_size]};
    NSData *const plaintext_data{[NSData dataWithBytes:plaintext
                                                length:plaintext_size]};
    NSData *const result{[SourcemetaCoreAESGCM sealWithKey:key_data
                                                     nonce:nonce_data
                                                 plaintext:plaintext_data]};
    if (output == nullptr || result == nil ||
        result.length != plaintext_size + 16) {
      return false;
    }

    std::memcpy(output, result.bytes, result.length);
    return true;
  }
}

extern "C" auto sourcemeta_core_aes_256_gcm_open_cryptokit(
    const unsigned char *key, std::size_t key_size, const unsigned char *nonce,
    std::size_t nonce_size, const unsigned char *ciphertext,
    std::size_t ciphertext_size, const unsigned char *tag,
    std::size_t tag_size, unsigned char *output) -> bool {
  @autoreleasepool {
    NSData *const key_data{[NSData dataWithBytes:key length:key_size]};
    NSData *const nonce_data{[NSData dataWithBytes:nonce length:nonce_size]};
    NSData *const ciphertext_data{[NSData dataWithBytes:ciphertext
                                                 length:ciphertext_size]};
    NSData *const tag_data{[NSData dataWithBytes:tag length:tag_size]};
    NSData *const result{[SourcemetaCoreAESGCM openWithKey:key_data
                                                     nonce:nonce_data
                                                ciphertext:ciphertext_data
                                                       tag:tag_data]};
    if (result == nil) {
      return false;
    }

    // The plaintext length always equals the ciphertext length for GCM, so a
    // differing result length would overflow the caller's output buffer
    if (result.length != ciphertext_size) {
      return false;
    }

    if (result.length > 0) {
      if (output == nullptr) {
        return false;
      }

      std::memcpy(output, result.bytes, result.length);
    }

    return true;
  }
}
