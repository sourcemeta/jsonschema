#include <sourcemeta/core/ip.h>

#include <array>   // std::array
#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

static constexpr auto make_hex_table() -> std::array<bool, 256> {
  std::array<bool, 256> table{};
  for (auto index{0u}; index < 256; index++) {
    table[index] = (index >= '0' && index <= '9') ||
                   (index >= 'a' && index <= 'f') ||
                   (index >= 'A' && index <= 'F');
  }
  return table;
}

static constexpr auto HEX_TABLE{make_hex_table()};

static constexpr auto is_hex_digit(const char character) -> bool {
  return HEX_TABLE[static_cast<std::uint8_t>(character)];
}

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

} // namespace sourcemeta::core
