#include <sourcemeta/core/numeric_parse.h>

#include <stdexcept> // std::invalid_argument, std::out_of_range

namespace sourcemeta::core {

auto to_double(const std::string &input) noexcept -> std::optional<double> {
  try {
    return std::stod(input);
  } catch (const std::invalid_argument &) {
    return std::nullopt;
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }
}

auto to_int64_t(const std::string &input) noexcept
    -> std::optional<std::int64_t> {
  try {
    return static_cast<std::int64_t>(std::stoll(input));
  } catch (const std::invalid_argument &) {
    return std::nullopt;
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }
}

} // namespace sourcemeta::core
