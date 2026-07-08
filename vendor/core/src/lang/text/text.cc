#include <sourcemeta/core/text.h>

#include <cstddef>     // std::size_t
#include <filesystem>  // std::filesystem::path
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair, std::move

namespace {

auto is_ascii_whitespace(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\n' ||
         character == '\v' || character == '\f' || character == '\r';
}

auto is_ascii_letter(const char character) noexcept -> bool {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z');
}

auto to_ascii_uppercase(const char character) noexcept -> char {
  return (character >= 'a' && character <= 'z')
             ? static_cast<char>(character - ('a' - 'A'))
             : character;
}

} // namespace

namespace sourcemeta::core {

auto to_lowercase(std::filesystem::path &value) -> void {
  auto native{value.native()};
  to_lowercase(native);
  value = std::filesystem::path{std::move(native)};
}

auto is_lowercase(const std::filesystem::path &value) noexcept -> bool {
  return is_lowercase(value.native());
}

auto to_title_case(std::string &value) -> void {
  std::size_t write{0};
  bool capitalize_next{true};
  bool pending_separator{false};
  for (const char character : value) {
    if (character == '_' || character == '-') {
      if (write > 0) {
        pending_separator = true;
      }
    } else {
      if (pending_separator) {
        value[write++] = ' ';
        pending_separator = false;
        capitalize_next = true;
      }
      if (capitalize_next) {
        value[write++] = to_ascii_uppercase(character);
        if (is_ascii_letter(character)) {
          capitalize_next = false;
        }
      } else {
        value[write++] = character;
      }
    }
  }
  value.resize(write);
}

auto truncate(std::string &input, const std::size_t maximum_length,
              const std::string_view marker) -> void {
  if (input.size() <= maximum_length) {
    return;
  }

  std::size_t boundary{maximum_length};
  while (boundary > 0 &&
         (static_cast<unsigned char>(input[boundary]) & 0xC0) == 0x80) {
    --boundary;
  }
  input.resize(boundary);
  input.append(marker);
}

auto trim(const std::string_view input) noexcept -> std::string_view {
  std::string_view result{input};
  while (!result.empty() && is_ascii_whitespace(result.front())) {
    result.remove_prefix(1);
  }
  while (!result.empty() && is_ascii_whitespace(result.back())) {
    result.remove_suffix(1);
  }
  return result;
}

auto strip_left(const std::string_view input, const char character) noexcept
    -> std::string_view {
  std::string_view result{input};
  while (!result.empty() && result.front() == character) {
    result.remove_prefix(1);
  }
  return result;
}

auto strip_right(const std::string_view input, const char character) noexcept
    -> std::string_view {
  std::string_view result{input};
  while (!result.empty() && result.back() == character) {
    result.remove_suffix(1);
  }
  return result;
}

auto pad_left(const std::string_view input, const std::size_t width,
              const char character) -> std::string {
  if (input.size() >= width) {
    return std::string{input};
  }

  return std::string(width - input.size(), character) + std::string{input};
}

auto take_until(const std::string_view input, const char marker) noexcept
    -> std::string_view {
  const auto position{input.find(marker)};
  if (position == std::string_view::npos) {
    return input;
  }
  std::string_view result{input};
  result.remove_suffix(input.size() - position);
  return result;
}

auto split_once(const std::string_view input, const char delimiter) noexcept
    -> std::optional<std::pair<std::string_view, std::string_view>> {
  const auto position{input.find(delimiter)};
  if (position == std::string_view::npos) {
    return std::nullopt;
  }
  std::string_view before{input};
  before.remove_suffix(input.size() - position);
  std::string_view after{input};
  after.remove_prefix(position + 1);
  return std::pair{before, after};
}

auto split_once(const std::string_view input,
                const std::string_view delimiter) noexcept
    -> std::optional<std::pair<std::string_view, std::string_view>> {
  if (delimiter.empty()) {
    return std::nullopt;
  }
  const auto position{input.find(delimiter)};
  if (position == std::string_view::npos) {
    return std::nullopt;
  }
  std::string_view before{input};
  before.remove_suffix(input.size() - position);
  std::string_view after{input};
  after.remove_prefix(position + delimiter.size());
  return std::pair{before, after};
}

auto squeeze(const std::string_view input, const char character,
             std::string &output) -> void {
  bool in_run{false};
  for (const auto value : input) {
    if (value == character) {
      if (!in_run) {
        output.push_back(value);
      }

      in_run = true;
    } else {
      output.push_back(value);
      in_run = false;
    }
  }
}

auto squeeze(const std::string_view input, const char character)
    -> std::string {
  std::string result;
  result.reserve(input.size());
  squeeze(input, character, result);
  return result;
}

auto remove_suffix_ignore_case(const std::string_view input,
                               const std::string_view suffix) noexcept
    -> std::string_view {
  if (suffix.empty() || suffix.size() > input.size()) {
    return input;
  }

  const std::size_t offset{input.size() - suffix.size()};
  for (std::size_t index{0}; index < suffix.size(); ++index) {
    if (to_lowercase(input[offset + index]) != to_lowercase(suffix[index])) {
      return input;
    }
  }

  std::string_view result{input};
  result.remove_suffix(suffix.size());
  return result;
}

auto hex_to_bytes(const std::string_view input, const bool allow_odd_length)
    -> std::optional<std::string> {
  const auto odd_length{input.size() % 2 != 0};
  if (odd_length && !allow_odd_length) {
    return std::nullopt;
  }

  std::string result;
  result.reserve(input.size() / 2 + 1);

  std::size_t index{0};
  if (odd_length) {
    const auto nibble{hex_digit_value(input[0])};
    if (nibble < 0) {
      return std::nullopt;
    }

    result.push_back(static_cast<char>(nibble));
    index = 1;
  }

  for (; index < input.size(); index += 2) {
    const auto high{hex_digit_value(input[index])};
    const auto low{hex_digit_value(input[index + 1])};
    if (high < 0 || low < 0) {
      return std::nullopt;
    }

    result.push_back(static_cast<char>((high << 4) | low));
  }

  return result;
}

auto bytes_to_hex(const std::string_view input) -> std::string {
  static constexpr std::string_view digits{"0123456789abcdef"};
  std::string result;
  result.reserve(input.size() * 2);
  for (const auto character : input) {
    const auto byte{static_cast<unsigned char>(character)};
    result.push_back(digits[byte >> 4u]);
    result.push_back(digits[byte & 0x0fu]);
  }

  return result;
}

} // namespace sourcemeta::core
