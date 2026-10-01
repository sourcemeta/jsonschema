#include <sourcemeta/core/numeric_parse.h>

#include <array>        // std::array
#include <cfenv>        // std::fegetround, FE_TONEAREST
#include <charconv>     // std::from_chars
#include <cstdint>      // std::uint64_t, std::int32_t
#include <limits>       // std::numeric_limits
#include <system_error> // std::errc

#if defined(__APPLE__)
#include <cerrno>  // errno, ERANGE
#include <cmath>   // HUGE_VAL
#include <cstring> // std::memcpy
#include <string>  // std::string
#include <xlocale.h>
#endif

namespace {

// Every power of ten up to this one is held exactly by a double, and so is
// every integer below two to the fifty-third
constexpr std::int32_t EXACT_POWER_LIMIT{22};
constexpr std::uint64_t EXACT_SIGNIFICAND_LIMIT{std::uint64_t{1} << 53};

// Nineteen digits is the most that cannot carry past the accumulator
constexpr std::int32_t MAXIMUM_ACCUMULATED_DIGITS{19};

// The longest input this can possibly recover: a sign, the digits a significand
// held exactly can carry, a decimal point among them, and an exponent inside
// the exact range together with its own sign. Anything longer is handed on
// without being walked, which also keeps the digit counters far away from their
// own limits however long the input is
constexpr std::size_t MAXIMUM_EXACT_LENGTH{25};

constexpr std::array<double, EXACT_POWER_LIMIT + 1> POWERS_OF_TEN{
    {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
     1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22}};

// Twenty digits cannot be held, and nineteen is the most that can be gathered
// without the accumulator carrying past itself
constexpr std::int32_t MAXIMUM_INTEGER_DIGITS{19};

// Gather a whole number without checking after every digit whether the
// accumulator has carried past itself, which is what the general conversion
// has to do because it cannot know how many digits are coming. Nineteen digits
// fit regardless, so one check at the end covers the whole run. Reporting no
// value leaves the input to the general conversion, which decides whether it is
// out of range or not a number at all
auto to_int64_t_narrow(const std::string_view input) noexcept
    -> std::optional<std::int64_t> {
  // Settled before a digit is read, so that a run far longer than this route
  // could ever hold is not walked only to be handed on anyway
  if (input.size() > static_cast<std::size_t>(MAXIMUM_INTEGER_DIGITS) + 1) {
    return std::nullopt;
  }

  const char *cursor{input.data()};
  const char *const end{cursor + input.size()};

  bool negative{false};
  if (cursor < end && *cursor == '-') {
    negative = true;
    cursor += 1;
  }

  std::uint64_t magnitude{0};
  std::int32_t digits{0};
  while (cursor < end && *cursor >= '0' && *cursor <= '9') {
    magnitude = (magnitude * 10) + static_cast<std::uint64_t>(*cursor - '0');
    digits += 1;
    cursor += 1;
  }

  if (digits == 0 || digits > MAXIMUM_INTEGER_DIGITS || cursor != end) {
    return std::nullopt;
  }

  // The negative side of the range reaches one further than the positive one,
  // and that one value cannot be reached by negating a signed quantity
  constexpr auto HIGHEST{
      static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())};
  if (negative) {
    if (magnitude > HIGHEST + 1) {
      return std::nullopt;
    }

    if (magnitude == HIGHEST + 1) {
      return std::numeric_limits<std::int64_t>::min();
    }

    return -static_cast<std::int64_t>(magnitude);
  }

  if (magnitude > HIGHEST) {
    return std::nullopt;
  }

  return static_cast<std::int64_t>(magnitude);
}

// Recover a number whose significand and whose power of ten are both held
// exactly, where scaling the one by the other is a single operation that IEEE
// 754 rounds once and therefore lands on the same value the general routine
// would reach the slow way. Reporting no value means the input is either
// outside that range or not a number at all, and either way the general routine
// decides what it is, so this never has to agree with it on a rejection
auto to_double_exact(const std::string_view input) noexcept
    -> std::optional<double> {
  if (input.size() > MAXIMUM_EXACT_LENGTH) {
    return std::nullopt;
  }

  const char *cursor{input.data()};
  const char *const end{cursor + input.size()};

  bool negative{false};
  if (cursor < end && *cursor == '-') {
    negative = true;
    cursor += 1;
  }

  std::uint64_t significand{0};
  std::int32_t digits{0};
  while (cursor < end && *cursor >= '0' && *cursor <= '9') {
    significand =
        (significand * 10) + static_cast<std::uint64_t>(*cursor - '0');
    digits += 1;
    cursor += 1;
  }

  // A number has to open with a digit, which also rejects a leading plus sign
  // and an empty input
  if (digits == 0) {
    return std::nullopt;
  }

  std::int32_t fraction_digits{0};
  if (cursor < end && *cursor == '.') {
    cursor += 1;
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
      significand =
          (significand * 10) + static_cast<std::uint64_t>(*cursor - '0');
      fraction_digits += 1;
      cursor += 1;
    }

    // A decimal point with nothing behind it is outside the grammar this
    // recognises, whatever the general routine makes of it
    if (fraction_digits == 0) {
      return std::nullopt;
    }
  }

  if (digits + fraction_digits > MAXIMUM_ACCUMULATED_DIGITS) {
    return std::nullopt;
  }

  std::int32_t exponent{0};
  if (cursor < end && (*cursor == 'e' || *cursor == 'E')) {
    cursor += 1;
    bool exponent_negative{false};
    if (cursor < end && (*cursor == '-' || *cursor == '+')) {
      exponent_negative = *cursor == '-';
      cursor += 1;
    }

    std::int32_t exponent_digits{0};
    while (cursor < end && *cursor >= '0' && *cursor <= '9') {
      // Stop accumulating well before the counter could carry past itself, as
      // a power this far out is beyond the exact range regardless
      if (exponent > EXACT_POWER_LIMIT) {
        return std::nullopt;
      }

      exponent = (exponent * 10) + (*cursor - '0');
      exponent_digits += 1;
      cursor += 1;
    }

    if (exponent_digits == 0) {
      return std::nullopt;
    }

    if (exponent_negative) {
      exponent = -exponent;
    }
  }

  if (cursor != end) {
    return std::nullopt;
  }

  const auto power{exponent - fraction_digits};
  if (significand >= EXACT_SIGNIFICAND_LIMIT || power > EXACT_POWER_LIMIT ||
      power < -EXACT_POWER_LIMIT) {
    return std::nullopt;
  }

  // A significand held exactly becomes a double without being rounded at all,
  // so a number that needs no scaling is recovered whatever rounding the
  // environment asks for. Scaling it is a single rounded operation, and that
  // one obeys the environment, while the general conversion is specified to
  // round to nearest regardless of it. So the shortcut only stands in for the
  // general conversion while the environment agrees with it
  auto value{static_cast<double>(significand)};
  if (power != 0) {
    if (std::fegetround() != FE_TONEAREST) {
      return std::nullopt;
    }

    if (power > 0) {
      value *= POWERS_OF_TEN[static_cast<std::size_t>(power)];
    } else {
      value /= POWERS_OF_TEN[static_cast<std::size_t>(-power)];
    }
  }

  return negative ? -value : value;
}

} // namespace

namespace sourcemeta::core {

auto to_double(const std::string_view input) noexcept -> std::optional<double> {
  // Most numbers a document carries are short enough to be recovered exactly
  // without consulting the general routine, and taking them here also spares
  // them the pass over the alphabet below
  const auto exact{to_double_exact(input)};
  if (exact.has_value()) {
    return exact;
  }

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
  static const locale_t C_LOCALE{newlocale(LC_ALL_MASK, "C", nullptr)};
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
  const auto value{strtod_l(start, &end, C_LOCALE)};
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
  const auto narrow{to_int64_t_narrow(input)};
  if (narrow.has_value()) {
    return narrow;
  }

  // Naming a radix reaches the general form of the conversion, which carries
  // that radix through every digit, while leaving it out reaches the form
  // specialised for ten. The two accept exactly the same inputs
  std::int64_t value{};
  const auto result{
      std::from_chars(input.data(), input.data() + input.size(), value)};
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
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
  std::uint32_t value{};
  const auto result{
      std::from_chars(input.data(), input.data() + input.size(), value)};
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
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
