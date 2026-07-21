#ifndef SOURCEMETA_CORE_CRYPTO_DER_H_
#define SOURCEMETA_CORE_CRYPTO_DER_H_

// Minimal DER (ITU-T X.690 distinguished encoding rules) reading and building
// primitives shared by the key parsing and native key construction paths

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair

namespace sourcemeta::core {

// A single parsed DER type-length-value element, along with the bytes that
// follow it within the same sequence
struct DERElement {
  unsigned char tag;
  std::string_view content;
  std::string_view rest;
};

// Read one DER element from the front of the input, following the definite
// length encoding of ITU-T X.690
inline auto der_read(const std::string_view input)
    -> std::optional<DERElement> {
  if (input.size() < 2) {
    return std::nullopt;
  }

  const auto tag{static_cast<unsigned char>(input[0])};
  std::size_t position{1};
  const auto first{static_cast<unsigned char>(input[position])};
  position += 1;
  std::size_t length{0};
  if (first < 0x80) {
    length = first;
  } else {
    const std::size_t count{first & 0x7fu};
    if (count == 0 || count > 4 || position + count > input.size()) {
      return std::nullopt;
    }

    // The distinguished encoding rules require the minimal number of length
    // octets (ITU-T X.690 Section 10.1), so a leading zero octet is not
    // canonical
    if (static_cast<unsigned char>(input[position]) == 0) {
      return std::nullopt;
    }

    for (std::size_t index{0}; index < count; ++index) {
      length = (length << 8u) | static_cast<unsigned char>(input[position]);
      position += 1;
    }

    // A value below the short-form limit must use the short form
    if (length < 0x80) {
      return std::nullopt;
    }
  }

  // Written so that the addition cannot overflow, as position never exceeds
  // the input size at this point
  if (length > input.size() - position) {
    return std::nullopt;
  }

  return DERElement{.tag = tag,
                    .content = input.substr(position, length),
                    .rest = input.substr(position + length)};
}

// Append the definite-form length octets of a DER element, using the minimal
// number of long-form octets for any length (ITU-T X.690 Section 10.1)
inline auto der_append_length(std::string &output, const std::size_t length)
    -> void {
  if (length < 128) {
    output.push_back(static_cast<char>(length));
    return;
  }

  std::string octets;
  for (std::size_t remaining{length}; remaining > 0; remaining >>= 8u) {
    octets.insert(octets.begin(), static_cast<char>(remaining & 0xffu));
  }

  output.push_back(static_cast<char>(0x80u | octets.size()));
  output.append(octets);
}

// Append a complete DER element, wrapping the content in its tag and length
inline auto der_append_element(std::string &output, const unsigned char tag,
                               const std::string_view content) -> void {
  output.push_back(static_cast<char>(tag));
  der_append_length(output, content.size());
  output.append(content);
}

// Append a DER INTEGER holding the big-endian unsigned value
inline auto der_append_unsigned_integer(std::string &output,
                                        std::string_view value) -> void {
  while (!value.empty() && value.front() == '\x00') {
    value.remove_prefix(1);
  }

  // A leading zero byte keeps the value positive when its high bit is set,
  // and represents the value zero when nothing remains
  const auto needs_zero_prefix{
      value.empty() ||
      (static_cast<unsigned char>(value.front()) & 0x80u) != 0};
  output.push_back('\x02');
  der_append_length(output, value.size() + (needs_zero_prefix ? 1 : 0));
  if (needs_zero_prefix) {
    output.push_back('\x00');
  }

  output.append(value);
}

// The minimal big-endian magnitude of a canonical non-negative DER INTEGER
// (ITU-T X.690 Section 8.3), returning no value for an empty, negative, or
// non-canonically sign-prefixed value, so a malformed integer cannot be
// silently reinterpreted as a different positive number
inline auto der_unsigned_integer(std::string_view content)
    -> std::optional<std::string_view> {
  if (content.empty()) {
    return std::nullopt;
  }

  // A leading octet with the high bit set encodes a negative value, and a
  // leading zero octet is only canonical when the next octet would otherwise
  // be read as negative
  if (static_cast<unsigned char>(content.front()) >= 0x80) {
    return std::nullopt;
  }

  if (content.size() >= 2 && content.front() == '\x00' &&
      static_cast<unsigned char>(content[1]) < 0x80) {
    return std::nullopt;
  }

  while (!content.empty() && content.front() == '\x00') {
    content.remove_prefix(1);
  }

  return content;
}

// Read the modulus and public exponent from a PKCS#1 RSAPublicKey structure
// (RFC 8017 Appendix A.1.1), a DER SEQUENCE of exactly the two integers, each
// returned as its minimal big-endian magnitude. Trailing bytes at either level
// are rejected so the input parses as exactly that structure
inline auto der_read_rsa_public_key(const std::string_view der)
    -> std::optional<std::pair<std::string, std::string>> {
  const auto sequence{der_read(der)};
  if (!sequence.has_value() || sequence->tag != 0x30 ||
      !sequence->rest.empty()) {
    return std::nullopt;
  }

  const auto modulus{der_read(sequence->content)};
  if (!modulus.has_value() || modulus->tag != 0x02) {
    return std::nullopt;
  }

  const auto exponent{der_read(modulus->rest)};
  if (!exponent.has_value() || exponent->tag != 0x02 ||
      !exponent->rest.empty()) {
    return std::nullopt;
  }

  const auto modulus_value{der_unsigned_integer(modulus->content)};
  const auto exponent_value{der_unsigned_integer(exponent->content)};
  if (!modulus_value.has_value() || !exponent_value.has_value()) {
    return std::nullopt;
  }

  return std::pair{std::string{modulus_value.value()},
                   std::string{exponent_value.value()}};
}

} // namespace sourcemeta::core

#endif
