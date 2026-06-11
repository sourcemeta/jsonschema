#include <sourcemeta/core/crypto_uuid.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace {

constexpr auto is_hex_digit(const char character) -> bool {
  return (character >= '0' && character <= '9') ||
         (character >= 'a' && character <= 'f') ||
         (character >= 'A' && character <= 'F');
}

} // namespace

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
#include <openssl/rand.h> // RAND_bytes
#include <stdexcept>      // std::runtime_error
#else
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution
#endif

namespace sourcemeta::core {

// See RFC 9562 Section 5.4
// Format: xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx
// where 4 is the version and N is the variant (8, 9, a, or b)
auto uuidv4() -> std::string {
  static constexpr std::string_view digits = "0123456789abcdef";
  static constexpr std::string_view variant_digits = "89ab";
  static constexpr std::array<bool, 16> dash = {
      {false, false, false, false, true, false, true, false, true, false, true,
       false, false, false, false, false}};

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
  std::array<unsigned char, 16> random_bytes{};
  if (RAND_bytes(random_bytes.data(), static_cast<int>(random_bytes.size())) !=
      1) {
    throw std::runtime_error("Could not generate random bytes with OpenSSL");
  }
#else
  thread_local std::random_device device;
  thread_local std::mt19937 generator{device()};
  std::uniform_int_distribution<decltype(digits)::size_type> distribution(0,
                                                                          15);
  std::uniform_int_distribution<decltype(variant_digits)::size_type>
      variant_distribution(0, 3);
#endif

  std::string result;
  result.reserve(36);
  for (std::size_t index = 0; index < dash.size(); ++index) {
    if (dash[index]) {
      result += '-';
    }

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
    const auto high_nibble = (random_bytes[index] >> 4u) & 0x0fu;
    const auto low_nibble = random_bytes[index] & 0x0fu;
#endif

    // RFC 9562 Section 5.4: version bits (48-51) must be 0b0100
    if (index == 6) {
      result += '4';
      // RFC 9562 Section 5.4: variant bits (64-65) must be 0b10
    } else if (index == 8) {
#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
      result += variant_digits[high_nibble & 0x03u];
#else
      result += variant_digits[variant_distribution(generator)];
#endif
    } else {
#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
      result += digits[high_nibble];
#else
      result += digits[distribution(generator)];
#endif
    }

#ifdef SOURCEMETA_CORE_CRYPTO_USE_SYSTEM_OPENSSL
    result += digits[low_nibble];
#else
    result += digits[distribution(generator)];
#endif
  }

  return result;
}

auto is_uuid_like(const std::string_view value) -> bool {
  // Layout: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (exactly 36 chars)
  if (value.size() != 36) {
    return false;
  }

  if (value[8] != '-' || value[13] != '-' || value[18] != '-' ||
      value[23] != '-') {
    return false;
  }

  for (std::string_view::size_type position{0}; position < 36; ++position) {
    if (position == 8 || position == 13 || position == 18 || position == 23) {
      continue;
    }
    if (!is_hex_digit(value[position])) {
      return false;
    }
  }

  return true;
}

} // namespace sourcemeta::core
