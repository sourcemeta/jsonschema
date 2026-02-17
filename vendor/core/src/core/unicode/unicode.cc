#include <sourcemeta/core/unicode.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t
#include <sstream> // std::istringstream, std::ostringstream

namespace sourcemeta::core {

auto codepoint_to_utf8(const char32_t codepoint, std::ostream &output) -> void {
  assert(codepoint <= 0x10FFFF);
  assert(codepoint < 0xD800 || codepoint > 0xDFFF);
  if (codepoint < 0x80) {
    output.put(static_cast<char>(codepoint));
  } else if (codepoint < 0x800) {
    output.put(static_cast<char>(0xC0 | (codepoint >> 6)));
    output.put(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint < 0x10000) {
    output.put(static_cast<char>(0xE0 | (codepoint >> 12)));
    output.put(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.put(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    output.put(static_cast<char>(0xF0 | (codepoint >> 18)));
    output.put(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    output.put(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.put(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

auto codepoint_to_utf8(const char32_t codepoint, std::string &output) -> void {
  assert(codepoint <= 0x10FFFF);
  assert(codepoint < 0xD800 || codepoint > 0xDFFF);
  if (codepoint < 0x80) {
    output.push_back(static_cast<char>(codepoint));
  } else if (codepoint < 0x800) {
    output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint < 0x10000) {
    output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  }
}

auto codepoint_to_utf8(const char32_t codepoint) -> std::string {
  std::string output;
  codepoint_to_utf8(codepoint, output);
  return output;
}

auto utf8_to_utf32(std::istream &input) -> std::optional<std::u32string> {
  std::u32string result;
  std::uint8_t byte{0};

  while (input.read(reinterpret_cast<char *>(&byte), 1)) {
    char32_t code_point{0};
    std::uint8_t continuation_count{0};
    char32_t minimum{0};

    if (byte < 0x80) {
      result.push_back(byte);
      continue;
    } else if ((byte & 0xE0) == 0xC0) {
      code_point = byte & 0x1F;
      continuation_count = 1;
      minimum = 0x80;
    } else if ((byte & 0xF0) == 0xE0) {
      code_point = byte & 0x0F;
      continuation_count = 2;
      minimum = 0x800;
    } else if ((byte & 0xF8) == 0xF0) {
      code_point = byte & 0x07;
      continuation_count = 3;
      minimum = 0x10000;
    } else {
      return std::nullopt;
    }

    for (std::uint8_t index = 0; index < continuation_count; ++index) {
      std::uint8_t continuation{0};
      if (!input.read(reinterpret_cast<char *>(&continuation), 1) ||
          (continuation & 0xC0) != 0x80) {
        return std::nullopt;
      }

      code_point = (code_point << 6) | (continuation & 0x3F);
    }

    if (code_point < minimum || code_point > 0x10FFFF ||
        (code_point >= 0xD800 && code_point <= 0xDFFF)) {
      return std::nullopt;
    }

    result.push_back(code_point);
  }

  if (!input.eof()) {
    return std::nullopt;
  }

  return result;
}

auto utf8_to_utf32(const std::string_view input)
    -> std::optional<std::u32string> {
  std::istringstream stream{std::string{input}};
  return utf8_to_utf32(stream);
}

} // namespace sourcemeta::core
