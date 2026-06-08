#include <sourcemeta/core/text.h>

#include <cctype>      // std::isalpha, std::toupper
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
        value[write++] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(character)));
        if (std::isalpha(static_cast<unsigned char>(character))) {
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

} // namespace sourcemeta::core
