#include "crypto_eddsa_apple.h"

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
