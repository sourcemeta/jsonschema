#include <sourcemeta/core/ip.h>

namespace sourcemeta::core {

static constexpr auto is_digit(const char character) -> bool {
  return character >= '0' && character <= '9';
}

auto is_ipv4(const std::string_view address) -> bool {
  if (address.empty()) {
    return false;
  }

  std::string_view::size_type position{0};
  unsigned int octet_count{0};

  while (octet_count < 4) {
    if (position >= address.size()) {
      return false;
    }

    if (!is_digit(address[position])) {
      return false;
    }

    const auto octet_start{position};
    unsigned int value{0};
    while (position < address.size() && is_digit(address[position])) {
      value = value * 10 + static_cast<unsigned int>(address[position] - '0');
      position += 1;
    }

    const auto octet_length{position - octet_start};

    if (octet_length > 1 && address[octet_start] == '0') {
      return false;
    }

    if (octet_length > 3 || value > 255) {
      return false;
    }

    octet_count += 1;

    if (octet_count < 4) {
      if (position >= address.size() || address[position] != '.') {
        return false;
      }
      position += 1;
    }
  }

  return position == address.size();
}

} // namespace sourcemeta::core
