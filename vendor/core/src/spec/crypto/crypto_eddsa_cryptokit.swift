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

// AES in Galois/Counter Mode, which the Apple Security framework does not
// expose through its C API either. CryptoKit provides it since macOS 10.15
@objc(SourcemetaCoreAESGCM)
public final class SourcemetaCoreAESGCM: NSObject {
  @objc public static func seal(key: Data, nonce: Data, plaintext: Data,
                                associatedData: Data) -> Data? {
    guard key.count == 16 || key.count == 24 || key.count == 32 else {
      return nil
    }

    guard let sealingNonce = try? AES.GCM.Nonce(data: nonce) else {
      return nil
    }

    guard let box = try? AES.GCM.seal(
        plaintext, using: SymmetricKey(data: key),
        nonce: sealingNonce, authenticating: associatedData) else {
      return nil
    }

    return box.ciphertext + box.tag
  }

  @objc public static func open(key: Data, nonce: Data, ciphertext: Data,
                                tag: Data, associatedData: Data) -> Data? {
    guard key.count == 16 || key.count == 24 || key.count == 32 else {
      return nil
    }

    guard let openingNonce = try? AES.GCM.Nonce(data: nonce),
          let box = try? AES.GCM.SealedBox(
              nonce: openingNonce, ciphertext: ciphertext, tag: tag) else {
      return nil
    }

    return try? AES.GCM.open(
        box, using: SymmetricKey(data: key), authenticating: associatedData)
  }
}

// HMAC-based key derivation (RFC 5869), which the Apple Security framework
// does not expose through its C API and CommonCrypto does not implement at
// all. CryptoKit provides it since macOS 11, so every entry point reports an
// absent result on an older system and the caller composes the derivation from
// HMAC instead
@objc(SourcemetaCoreHKDF)
public final class SourcemetaCoreHKDF: NSObject {
  @available(macOS 11.0, *)
  private static func derive<H: HashFunction>(
      _ hash: H.Type, inputKeyMaterial: Data, salt: Data, info: Data,
      outputByteCount: Int) -> Data {
    let key = HKDF<H>.deriveKey(
        inputKeyMaterial: SymmetricKey(data: inputKeyMaterial), salt: salt,
        info: info, outputByteCount: outputByteCount)
    return key.withUnsafeBytes { Data($0) }
  }

  @available(macOS 11.0, *)
  private static func extract<H: HashFunction>(
      _ hash: H.Type, inputKeyMaterial: Data, salt: Data) -> Data {
    let code = HKDF<H>.extract(
        inputKeyMaterial: SymmetricKey(data: inputKeyMaterial), salt: salt)
    return code.withUnsafeBytes { Data($0) }
  }

  @available(macOS 11.0, *)
  private static func expand<H: HashFunction>(
      _ hash: H.Type, pseudoRandomKey: Data, info: Data,
      outputByteCount: Int) -> Data {
    let key = HKDF<H>.expand(
        pseudoRandomKey: pseudoRandomKey, info: info,
        outputByteCount: outputByteCount)
    return key.withUnsafeBytes { Data($0) }
  }

  @objc public static func deriveKey(hash: Int, inputKeyMaterial: Data,
                                     salt: Data, info: Data,
                                     outputByteCount: Int) -> Data? {
    guard #available(macOS 11.0, *) else {
      return nil
    }

    switch hash {
    case 0:
      return derive(SHA256.self, inputKeyMaterial: inputKeyMaterial,
                    salt: salt, info: info, outputByteCount: outputByteCount)
    case 1:
      return derive(SHA384.self, inputKeyMaterial: inputKeyMaterial,
                    salt: salt, info: info, outputByteCount: outputByteCount)
    case 2:
      return derive(SHA512.self, inputKeyMaterial: inputKeyMaterial,
                    salt: salt, info: info, outputByteCount: outputByteCount)
    default:
      return nil
    }
  }

  @objc public static func extractKey(hash: Int, inputKeyMaterial: Data,
                                      salt: Data) -> Data? {
    guard #available(macOS 11.0, *) else {
      return nil
    }

    switch hash {
    case 0:
      return extract(SHA256.self, inputKeyMaterial: inputKeyMaterial,
                     salt: salt)
    case 1:
      return extract(SHA384.self, inputKeyMaterial: inputKeyMaterial,
                     salt: salt)
    case 2:
      return extract(SHA512.self, inputKeyMaterial: inputKeyMaterial,
                     salt: salt)
    default:
      return nil
    }
  }

  @objc public static func expandKey(hash: Int, pseudoRandomKey: Data,
                                     info: Data,
                                     outputByteCount: Int) -> Data? {
    guard #available(macOS 11.0, *) else {
      return nil
    }

    switch hash {
    case 0:
      return expand(SHA256.self, pseudoRandomKey: pseudoRandomKey, info: info,
                    outputByteCount: outputByteCount)
    case 1:
      return expand(SHA384.self, pseudoRandomKey: pseudoRandomKey, info: info,
                    outputByteCount: outputByteCount)
    case 2:
      return expand(SHA512.self, pseudoRandomKey: pseudoRandomKey, info: info,
                    outputByteCount: outputByteCount)
    default:
      return nil
    }
  }
}
