#include <sourcemeta/core/numeric_parse.h>

#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <stdexcept>    // std::invalid_argument, std::out_of_range
#include <system_error> // std::errc

namespace sourcemeta::core {

auto to_double(const std::string &input) noexcept -> std::optional<double> {
  try {
    std::size_t position{0};
    const auto value{std::stod(input, &position)};
    if (position != input.size()) {
      return std::nullopt;
    }
    return value;
  } catch (const std::invalid_argument &) {
    return std::nullopt;
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }
}

auto to_int64_t(const std::string &input) noexcept
    -> std::optional<std::int64_t> {
  return to_int64_t(input, 10);
}

auto to_int64_t(const std::string &input, const int base) noexcept
    -> std::optional<std::int64_t> {
  std::int64_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value, base);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

auto to_uint64_t(const std::string &input) noexcept
    -> std::optional<std::uint64_t> {
  std::uint64_t value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), value);
  if (result.ec != std::errc{} || result.ptr != input.data() + input.size()) {
    return std::nullopt;
  }

  return value;
}

} // namespace sourcemeta::core
