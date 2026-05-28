#include <sourcemeta/core/text.h>

#include <cctype>      // std::isalpha, std::toupper
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto to_lowercase(const char character) noexcept -> char {
  return (character >= 'A' && character <= 'Z')
             ? static_cast<char>(character + 32)
             : character;
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
