#include <sourcemeta/core/ip.h>
#include <sourcemeta/core/text.h>

#include "ip_helpers.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto is_ipv6(const std::string_view address) -> bool {
  if (address.empty()) {
    return false;
  }

  const auto size{address.size()};

  if (address.front() == '[' || address.back() == ']') {
    return false;
  }

  const auto double_colon{address.find("::")};
  const bool has_compression{double_colon != std::string_view::npos};

  if (has_compression &&
      address.find("::", double_colon + 2) != std::string_view::npos) {
    return false;
  }

  if (address.front() == ':' && (!has_compression || double_colon != 0)) {
    return false;
  }
  if (address.back() == ':' &&
      (!has_compression || double_colon + 1 != size - 1)) {
    return false;
  }

  unsigned int group_count{0};
  std::string_view::size_type position{0};

  while (position < size) {
    if (has_compression && position == double_colon) {
      position += 2;
      continue;
    }

    const auto group_start{position};
    unsigned int hex_count{0};
    bool found_dot{false};

    while (position < size) {
      const auto character{address[position]};
      if (character == ':') {
        break;
      }
      if (character == '.') {
        found_dot = true;
        break;
      }
      if (!is_hex_digit(character)) {
        return false;
      }
      hex_count += 1;
      position += 1;
    }

    if (found_dot) {
      if (!is_ipv4(address.substr(group_start))) {
        return false;
      }
      group_count += 2;
      break;
    }

    if (hex_count == 0 || hex_count > 4) {
      return false;
    }

    group_count += 1;

    if (position < size && address[position] == ':') {
      if (has_compression && position == double_colon) {
        continue;
      }
      position += 1;
      if (position >= size) {
        return false;
      }
    }
  }

  if (has_compression) {
    return group_count < 8;
  }

  return group_count == 8;
}

namespace {

// Append the big-endian bytes of a colon-separated run of hex groups, where the
// final group may be an embedded IPv4 dotted quad, to a buffer at an offset,
// returning how many bytes were written. The run comes from a valid address.
auto fill_ipv6_run(const std::string_view run,
                   std::array<std::uint8_t, 16> &output,
                   const std::size_t offset) -> std::size_t {
  if (run.empty()) {
    return 0;
  }

  std::size_t length{offset};
  std::size_t position{0};
  while (true) {
    const auto colon{run.find(':', position)};
    const auto piece{run.substr(position, colon == std::string_view::npos
                                              ? std::string_view::npos
                                              : colon - position)};
    if (piece.find('.') != std::string_view::npos) {
      for (const auto octet : ipv4_octets(piece)) {
        output[length] = octet;
        length += 1;
      }
    } else {
      std::uint16_t value{0};
      for (const auto character : piece) {
        value = static_cast<std::uint16_t>(
            (value << 4U) |
            static_cast<std::uint16_t>(hex_digit_value(character)));
      }

      output[length] = static_cast<std::uint8_t>(value >> 8U);
      output[length + 1] = static_cast<std::uint8_t>(value & 0xffU);
      length += 2;
    }

    if (colon == std::string_view::npos) {
      break;
    }

    position = colon + 1;
  }

  return length - offset;
}

// Extract the sixteen bytes of a valid IPv6 address (RFC 4291 Section 2.2),
// expanding a "::" by zero-filling between the head and the tail
auto ipv6_bytes(const std::string_view address)
    -> std::array<std::uint8_t, 16> {
  std::array<std::uint8_t, 16> bytes{};
  const auto compression{address.find("::")};
  if (compression == std::string_view::npos) {
    fill_ipv6_run(address, bytes, 0);
    return bytes;
  }

  fill_ipv6_run(address.substr(0, compression), bytes, 0);

  std::array<std::uint8_t, 16> tail{};
  const auto tail_length{
      fill_ipv6_run(address.substr(compression + 2), tail, 0)};
  for (std::size_t index = 0; index < tail_length; index += 1) {
    bytes[16 - tail_length + index] = tail[index];
  }

  return bytes;
}

} // namespace

auto ipv6_classify(const std::string_view address)
    -> std::optional<IPAddressClass> {
  if (!is_ipv6(address)) {
    return std::nullopt;
  }

  const auto bytes{ipv6_bytes(address)};

  // The first ten bytes are zero for the IPv4-mapped and the deprecated
  // IPv4-compatible forms, both of which embed an IPv4 address (RFC 4291
  // Section 2.5.5)
  bool leading_zero{true};
  for (std::size_t index = 0; index < 10; index += 1) {
    if (bytes[index] != 0) {
      leading_zero = false;
      break;
    }
  }

  const std::array<std::uint8_t, 4> embedded{
      {bytes[12], bytes[13], bytes[14], bytes[15]}};

  // RFC 4291 Section 2.5.5.2: an IPv4-mapped address ::ffff:0:0/96 is
  // classified as its embedded IPv4 address, so ::ffff:127.0.0.1 cannot bypass
  // a check
  if (leading_zero && bytes[10] == 0xff && bytes[11] == 0xff) {
    return ipv4_classify_octets(embedded);
  }

  if (leading_zero && bytes[10] == 0 && bytes[11] == 0) {
    // RFC 4291 Section 2.5.2: the unspecified address ::
    if (embedded[0] == 0 && embedded[1] == 0 && embedded[2] == 0 &&
        embedded[3] == 0) {
      return IPAddressClass::Unspecified;
    }

    // RFC 4291 Section 2.5.3: the loopback address ::1
    if (embedded[0] == 0 && embedded[1] == 0 && embedded[2] == 0 &&
        embedded[3] == 1) {
      return IPAddressClass::Loopback;
    }

    // RFC 4291 Section 2.5.5.1: the deprecated IPv4-compatible form embeds an
    // IPv4 address, classified as that address so ::127.0.0.1 cannot bypass a
    // check
    return ipv4_classify_octets(embedded);
  }

  // RFC 4291 Section 2.7: multicast, ff00::/8
  if (bytes[0] == 0xff) {
    return IPAddressClass::Multicast;
  }

  // RFC 4291 Section 2.5.6: link-local unicast, fe80::/10
  if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80) {
    return IPAddressClass::LinkLocal;
  }

  // RFC 4193 Section 3: unique local, fc00::/7
  if ((bytes[0] & 0xfe) == 0xfc) {
    return IPAddressClass::UniqueLocal;
  }

  // RFC 4291 Section 2.4: only global unicast, 2000::/3, is globally reachable,
  // so every other prefix, including the IETF-reserved 0000::/8, is not
  if ((bytes[0] & 0xe0) != 0x20) {
    return IPAddressClass::Reserved;
  }

  // RFC 2928: the IETF protocol assignments block, 2001::/23, which the IANA
  // IPv6 Special-Purpose Address Registry marks as not globally reachable
  // unless a more specific allocation within it says otherwise
  if (bytes[0] == 0x20 && bytes[1] == 0x01 && (bytes[2] & 0xfe) == 0x00) {
    // RFC 7723, RFC 8155, and RFC 9665: the globally reachable anycast
    // addresses 2001:1::1, 2001:1::2, and 2001:1::3
    if (bytes[2] == 0x00 && bytes[3] == 0x01 && bytes[15] >= 1 &&
        bytes[15] <= 3) {
      bool anycast{true};
      for (std::size_t index = 4; index < 15; index += 1) {
        if (bytes[index] != 0) {
          anycast = false;
          break;
        }
      }
      if (anycast) {
        return IPAddressClass::Public;
      }
    }

    // RFC 7450: AMT, 2001:3::/32, globally reachable
    if (bytes[2] == 0x00 && bytes[3] == 0x03) {
      return IPAddressClass::Public;
    }

    // RFC 7535: AS112-v6, 2001:4:112::/48, globally reachable
    if (bytes[2] == 0x00 && bytes[3] == 0x04 && bytes[4] == 0x01 &&
        bytes[5] == 0x12) {
      return IPAddressClass::Public;
    }

    // RFC 7343: ORCHIDv2, 2001:20::/28, and RFC 9374: Drone Remote ID
    // Protocol Entity Tags, 2001:30::/28, both globally reachable
    if (bytes[2] == 0x00 && (bytes[3] & 0xe0) == 0x20) {
      return IPAddressClass::Public;
    }

    // Everything else in 2001::/23, including TEREDO (RFC 4380), 2001::/32,
    // benchmarking (RFC 5180), 2001:2::/48, and the deprecated ORCHID
    // (RFC 4843), 2001:10::/28, is not globally reachable
    return IPAddressClass::Reserved;
  }

  // RFC 3849: documentation, 2001:db8::/32, within global unicast
  if (bytes[0] == 0x20 && bytes[1] == 0x01 && bytes[2] == 0x0d &&
      bytes[3] == 0xb8) {
    return IPAddressClass::Reserved;
  }

  // RFC 3056 Section 2: a 6to4 address, 2002::/16, embeds the IPv4 address of
  // its tunnel endpoint, so it is classified as that address and a 6to4 form
  // of a private or loopback IPv4 address cannot bypass a check. Note that
  // RFC 7526 Section 4 deprecated the relay anycast mechanism, not unicast
  // 6to4 or its prefix
  if (bytes[0] == 0x20 && bytes[1] == 0x02) {
    return ipv4_classify_octets({{bytes[2], bytes[3], bytes[4], bytes[5]}});
  }

  // RFC 9637: documentation, 3fff::/20, within global unicast
  if (bytes[0] == 0x3f && bytes[1] == 0xff && (bytes[2] & 0xf0) == 0x00) {
    return IPAddressClass::Reserved;
  }

  return IPAddressClass::Public;
}

} // namespace sourcemeta::core
