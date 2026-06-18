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
}
