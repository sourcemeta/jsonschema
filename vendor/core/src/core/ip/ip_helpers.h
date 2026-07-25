#ifndef SOURCEMETA_CORE_IP_HELPERS_H_
#define SOURCEMETA_CORE_IP_HELPERS_H_

#include <sourcemeta/core/ip.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Extract the four octets of a valid IPv4 address (RFC 3986 Section 3.2.2)
inline auto ipv4_octets(const std::string_view address)
    -> std::array<std::uint8_t, 4> {
  std::array<std::uint8_t, 4> octets{};
  std::size_t index{0};
  unsigned int value{0};
  for (const auto character : address) {
    if (character == '.') {
      octets[index] = static_cast<std::uint8_t>(value);
      index += 1;
      value = 0;
    } else {
      value = (value * 10) + static_cast<unsigned int>(character - '0');
    }
  }

  octets[index] = static_cast<std::uint8_t>(value);
  return octets;
}

// Classify an IPv4 address from its octets (RFC 6890), shared so an IPv4-mapped
// IPv6 address can be classified as its embedded IPv4 address
inline auto ipv4_classify_octets(const std::array<std::uint8_t, 4> &octets)
    -> IPAddressClass {
  const auto first{octets[0]};
  const auto second{octets[1]};
  const auto third{octets[2]};

  // RFC 1122 Section 3.2.1.3: "this host on this network", 0.0.0.0/8
  if (first == 0) {
    return IPAddressClass::Unspecified;
  }

  // RFC 1122 Section 3.2.1.3: loopback, 127.0.0.0/8
  if (first == 127) {
    return IPAddressClass::Loopback;
  }

  // RFC 1918 Section 3: private-use, 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
  if (first == 10 || (first == 172 && second >= 16 && second <= 31) ||
      (first == 192 && second == 168)) {
    return IPAddressClass::Private;
  }

  // RFC 6598: shared address space, 100.64.0.0/10
  if (first == 100 && second >= 64 && second <= 127) {
    return IPAddressClass::Private;
  }

  // RFC 3927 Section 2.1: link-local, 169.254.0.0/16
  if (first == 169 && second == 254) {
    return IPAddressClass::LinkLocal;
  }

  // RFC 6890, RFC 5737, and RFC 2544: special-purpose ranges that are not
  // globally reachable
  if ((first == 192 && second == 0 && third == 0) ||
      (first == 192 && second == 0 && third == 2) ||
      (first == 198 && (second == 18 || second == 19)) ||
      (first == 198 && second == 51 && third == 100) ||
      (first == 203 && second == 0 && third == 113)) {
    return IPAddressClass::Reserved;
  }

  // RFC 5771: multicast, 224.0.0.0/4
  if (first >= 224 && first <= 239) {
    return IPAddressClass::Multicast;
  }

  // RFC 1112 Section 4: reserved for future use, 240.0.0.0/4, which subsumes
  // the 255.255.255.255 limited broadcast address
  if (first >= 240) {
    return IPAddressClass::Reserved;
  }

  return IPAddressClass::Public;
}

} // namespace sourcemeta::core

#endif
