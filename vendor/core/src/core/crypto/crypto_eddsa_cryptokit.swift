import CryptoKit
import Foundation

// The Ed25519 verification primitive that the Apple Security framework does
// not expose through its C API. CryptoKit provides it since macOS 10.15, and
// this class surfaces it to the Objective-C++ bridge through the generated
// Objective-C interface header
@objc(SourcemetaCoreEd25519)
public final class SourcemetaCoreEd25519: NSObject {
  @objc public static func verify(publicKey: Data, message: Data,
                                  signature: Data) -> Bool {
    guard let key = try? Curve25519.Signing.PublicKey(
        rawRepresentation: publicKey) else {
      return false
    }

    return key.isValidSignature(signature, for: message)
  }

  @objc public static func sign(seed: Data, message: Data) -> Data? {
    guard let key = try? Curve25519.Signing.PrivateKey(
        rawRepresentation: seed) else {
      return nil
    }

    return try? key.signature(for: message)
  }
}

// AES-256 in Galois/Counter Mode, which the Apple Security framework does not
// expose through its C API either. CryptoKit provides it since macOS 10.15
@objc(SourcemetaCoreAESGCM)
public final class SourcemetaCoreAESGCM: NSObject {
  @objc public static func seal(key: Data, nonce: Data,
                                plaintext: Data) -> Data? {
    guard key.count == 32 else {
      return nil
    }

    guard let sealingNonce = try? AES.GCM.Nonce(data: nonce) else {
      return nil
    }

    guard let box = try? AES.GCM.seal(
        plaintext, using: SymmetricKey(data: key),
        nonce: sealingNonce) else {
      return nil
    }

    return box.ciphertext + box.tag
  }

  @objc public static func open(key: Data, nonce: Data, ciphertext: Data,
                                tag: Data) -> Data? {
    guard key.count == 32 else {
      return nil
    }

    guard let openingNonce = try? AES.GCM.Nonce(data: nonce),
          let box = try? AES.GCM.SealedBox(
              nonce: openingNonce, ciphertext: ciphertext, tag: tag) else {
      return nil
    }

    return try? AES.GCM.open(box, using: SymmetricKey(data: key))
  }
}
