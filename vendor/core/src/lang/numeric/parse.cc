#include <sourcemeta/core/numeric_parse.h>

#include <charconv>     // std::from_chars
#include <system_error> // std::errc

#if defined(__APPLE__)
#include <array>   // std::array
#include <cerrno>  // errno, ERANGE
#include <cmath>   // HUGE_VAL
#include <cstring> // std::memcpy
#include <string>  // std::string
#include <xlocale.h>
#endif

namespace sourcemeta::core {

auto to_double(const std::string_view input) noexcept -> std::optional<double> {
  if (input.empty() || input.front() == '+') {
    return std::nullopt;
  }

  // Restrict the accepted alphabet upfront so that every platform rejects
  // leading whitespace, hexadecimal literals, and infinity or not-a-number
  // spellings in the same way
  for (const auto character : input) {
    const auto is_digit{character >= '0' && character <= '9'};
    if (!is_digit && character != '.' && character != '-' && character != '+' &&
        character != 'e' && character != 'E') {
      return std::nullopt;
    }
  }

#if defined(__APPLE__)
  // Apple's standard library cannot parse floating point values out of
  // character ranges until macOS 26, so older deployment targets rely on the
  // C library with an explicit locale to stay locale-independent. The input
  // is not guaranteed to be null-terminated, so it must be bounded first.
  // Darwin documents that a null locale argument selects the C locale, so a
  // failed locale construction still yields the intended behavior
  static const locale_t c_locale{newlocale(LC_ALL_MASK, "C", nullptr)};
  std::array<char, 64> buffer;
  std::string overflow;
  const char *start{nullptr};
  if (input.size() < buffer.size()) {
    std::memcpy(buffer.data(), input.data(), input.size());
    buffer[input.size()] = '\0';
    start = buffer.data();
  } else {
    try {
      overflow.assign(input);
    } catch (...) {
      return std::nullopt;
    }

    start = overflow.c_str();
  }

  errno = 0;
  char *end{nullptr};
  const auto value{strtod_l(start, &end, c_locale)};
  if (end != start + input.size()) {
    return std::nullopt;
  }

  // The C library may report a range error for subnormal results, which are
  // representable and accepted by std::from_chars on other platforms, so a
  // range error only counts when the result overflows to infinity or
  // underflows all the way to zero
  if (errno == ERANGE &&
      (value == HUGE_VAL || value == -HUGE_VAL || value == 0.0)) {
    return std::nullopt;
  }

  return value;
#else
  double value{};
  const auto result{
      std::from_chars(input.data(), input.data() + input.size(), value)};
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
#endif
}

auto to_int64_t(const std::string_view input) noexcept
    -> std::optional<std::int64_t> {
  return to_int64_t(input, 10);
}

auto to_int64_t(const std::string_view input, const int base) noexcept
    -> std::optional<std::int64_t> {
  std::int64_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value, base);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

auto to_uint64_t(const std::string_view input) noexcept
    -> std::optional<std::uint64_t> {
  std::uint64_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

auto to_uint32_t(const std::string_view input) noexcept
    -> std::optional<std::uint32_t> {
  return to_uint32_t(input, 10);
}

auto to_uint32_t(const std::string_view input, const int base) noexcept
    -> std::optional<std::uint32_t> {
  std::uint32_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value, base);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

auto to_uint16_t(const std::string_view input) noexcept
    -> std::optional<std::uint16_t> {
  std::uint16_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

} // namespace sourcemeta::core
