#include <sourcemeta/blaze/codegen.h>

namespace {

auto is_alpha(char character) -> bool {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z');
}

auto is_digit(char character) -> bool {
  return character >= '0' && character <= '9';
}

auto to_upper(char character) -> char {
  if (character >= 'a' && character <= 'z') {
    return static_cast<char>(character - 'a' + 'A');
  }
  return character;
}

auto symbol_to_identifier(const std::string_view prefix,
                          const std::vector<std::string> &symbol)
    -> std::string {
  std::string result{prefix};

  for (const auto &segment : symbol) {
    if (segment.empty()) {
      continue;
    }

    bool at_word_start{true};
    bool at_segment_start{true};
    for (const auto character : segment) {
      if (is_alpha(character)) {
        result += at_word_start ? to_upper(character) : character;
        at_word_start = false;
        at_segment_start = false;
      } else if (is_digit(character)) {
        if (at_segment_start) {
          result += '_';
        }
        result += character;
        at_word_start = false;
        at_segment_start = false;
      } else if (character == '_' || character == '$') {
        result += character;
        at_word_start = false;
        at_segment_start = false;
      } else {
        at_word_start = true;
      }
    }
  }

  if (result.empty()) {
    return "_";
  }

  if (is_digit(result[0])) {
    result.insert(0, "_");
  }

  return result;
}

} // namespace

namespace sourcemeta::blaze {

auto mangle(const std::string_view prefix,
            const sourcemeta::core::Pointer &pointer,
            const std::vector<std::string> &symbol,
            std::map<std::string, sourcemeta::core::Pointer> &cache)
    -> const std::string & {
  auto name{symbol_to_identifier(prefix, symbol)};

  while (true) {
    auto iterator{cache.find(name)};
    if (iterator != cache.end()) {
      if (iterator->second == pointer) {
        return iterator->first;
      }

      name.insert(0, "_");
    } else {
      auto result{cache.insert({std::move(name), pointer})};
      return result.first->first;
    }
  }
}

} // namespace sourcemeta::blaze
