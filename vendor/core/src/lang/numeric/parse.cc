#include <sourcemeta/core/numeric_parse.h>

#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <system_error> // std::errc

namespace sourcemeta::core {

auto to_double(const std::string_view input) noexcept -> std::optional<double> {
  try {
    const std::string owned{input};
    std::size_t position{0};
    const auto value{std::stod(owned, &position)};
    if (position != owned.size()) {
      return std::nullopt;
    }
    return value;
  } catch (...) {
    return std::nullopt;
  }
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

} // namespace sourcemeta::core
