#include <sourcemeta/core/unicode.h>

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string, std::wstring
#include <string_view> // std::string_view, std::wstring_view

#if defined(_WIN32) || defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <limits>    // std::numeric_limits
#include <windows.h> // MultiByteToWideChar, WideCharToMultiByte, CP_UTF8
#endif

#include "unicode_data.h"

namespace sourcemeta::core {

namespace {

// Decode the code point of a multi-byte sequence from its lead byte and the
// continuation bytes that follow it, rejecting invalid continuation bytes,
// overlong encodings, and code points that are not valid scalar values
auto utf8_decode_sequence(const std::uint8_t lead, const std::uint8_t size,
                          const std::uint8_t *continuations)
    -> std::optional<char32_t> {
  char32_t code_point{0};
  char32_t minimum{0};
  if (size == 2) {
    code_point = lead & 0x1F;
    minimum = 0x80;
  } else if (size == 3) {
    code_point = lead & 0x0F;
    minimum = 0x800;
  } else {
    code_point = lead & 0x07;
    minimum = 0x10000;
  }

  for (std::uint8_t index{0}; index < size - 1; ++index) {
    const auto continuation{continuations[index]};
    if (!is_utf8_continuation(continuation)) {
      return std::nullopt;
    }

    code_point = (code_point << 6) | (continuation & 0x3F);
  }

  if (code_point < minimum || !is_valid_codepoint(code_point)) {
    return std::nullopt;
  }

  return code_point;
}

// Append the UTF-8 byte sequence for `codepoint` by magnitude, without
// validating that it is a Unicode scalar value. A surrogate falls into the
// three-byte branch and produces its (ill-formed) WTF-8 encoding. The codepoint
// must be within the Unicode codespace, otherwise the four-byte branch would
// overflow the lead byte.
auto append_utf8(const char32_t codepoint, std::string &output) -> void {
  assert(codepoint <= 0x10FFFF);
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

} // namespace

auto codepoint_to_utf8(const char32_t codepoint, std::ostream &output) -> void {
  assert(is_valid_codepoint(codepoint));
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
  assert(is_valid_codepoint(codepoint));
  append_utf8(codepoint, output);
}

auto codepoint_to_utf8(const char32_t codepoint) -> std::string {
  std::string output;
  codepoint_to_utf8(codepoint, output);
  return output;
}

auto utf32_to_utf8(const std::u32string_view input) -> std::string {
  std::string output;
  for (const auto codepoint : input) {
    codepoint_to_utf8(codepoint, output);
  }
  return output;
}

auto utf32_to_utf8_lenient(const std::u32string_view input) -> std::string {
  std::string output;
  for (const auto codepoint : input) {
    append_utf8(codepoint, output);
  }
  return output;
}

auto utf8_to_utf32(std::istream &input) -> std::optional<std::u32string> {
  std::u32string result;
  std::uint8_t byte{0};

  while (input.read(reinterpret_cast<char *>(&byte), 1)) {
    const auto size{utf8_lead_byte_size(byte)};
    if (size == 0) {
      return std::nullopt;
    }
    if (size == 1) {
      result.push_back(byte);
      continue;
    }

    std::array<std::uint8_t, 3> continuations{};
    for (std::uint8_t index{0}; index < size - 1; ++index) {
      if (!input.read(reinterpret_cast<char *>(&continuations[index]), 1)) {
        return std::nullopt;
      }
    }

    const auto code_point{
        utf8_decode_sequence(byte, size, continuations.data())};
    if (!code_point.has_value()) {
      return std::nullopt;
    }

    result.push_back(code_point.value());
  }

  if (!input.eof()) {
    return std::nullopt;
  }

  return result;
}

auto utf8_to_utf32(const std::string_view input)
    -> std::optional<std::u32string> {
  std::u32string result;
  result.reserve(input.size());

  std::size_t position{0};
  while (position < input.size()) {
    const auto byte{static_cast<std::uint8_t>(input[position])};
    const auto size{utf8_lead_byte_size(byte)};
    if (size == 0) {
      return std::nullopt;
    }
    if (size == 1) {
      result.push_back(byte);
      position += 1;
      continue;
    }

    if (input.size() - position < size) {
      return std::nullopt;
    }

    const auto code_point{utf8_decode_sequence(
        byte, size,
        reinterpret_cast<const std::uint8_t *>(input.data() + position + 1))};
    if (!code_point.has_value()) {
      return std::nullopt;
    }

    result.push_back(code_point.value());
    position += size;
  }

  return result;
}

auto utf8_to_wide(const std::string_view input) -> std::wstring {
  if (input.empty()) {
    return L"";
  }

#if defined(_WIN32) || defined(__CYGWIN__)
  assert(input.size() <=
         static_cast<std::size_t>(std::numeric_limits<int>::max()));
  const auto size{MultiByteToWideChar(
      CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0)};
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                      result.data(), size);
  return result;
#else
  // Outside of Windows, a wide character holds an entire codepoint, so the
  // result never has more characters than the input has bytes
  static_assert(sizeof(wchar_t) >= 4,
                "a wide character must hold an entire codepoint");
  std::wstring result(input.size(), L'\0');
  std::size_t write{0};
  std::size_t read{0};
  while (read < input.size()) {
    const auto lead{static_cast<std::uint8_t>(input[read])};
    if (lead < 0x80) {
      result[write++] = static_cast<wchar_t>(lead);
      read += 1;
    } else if (lead < 0xE0) {
      result[write++] = static_cast<wchar_t>(
          ((lead & 0x1FU) << 6U) |
          (static_cast<std::uint8_t>(input[read + 1]) & 0x3FU));
      read += 2;
    } else if (lead < 0xF0) {
      result[write++] = static_cast<wchar_t>(
          ((lead & 0x0FU) << 12U) |
          ((static_cast<std::uint8_t>(input[read + 1]) & 0x3FU) << 6U) |
          (static_cast<std::uint8_t>(input[read + 2]) & 0x3FU));
      read += 3;
    } else {
      result[write++] = static_cast<wchar_t>(
          ((lead & 0x07U) << 18U) |
          ((static_cast<std::uint8_t>(input[read + 1]) & 0x3FU) << 12U) |
          ((static_cast<std::uint8_t>(input[read + 2]) & 0x3FU) << 6U) |
          (static_cast<std::uint8_t>(input[read + 3]) & 0x3FU));
      read += 4;
    }
  }

  result.resize(write);
  return result;
#endif
}

auto wide_to_utf8(const std::wstring_view input) -> std::string {
  if (input.empty()) {
    return "";
  }

#if defined(_WIN32) || defined(__CYGWIN__)
  assert(input.size() <=
         static_cast<std::size_t>(std::numeric_limits<int>::max()));
  const auto size{WideCharToMultiByte(CP_UTF8, 0, input.data(),
                                      static_cast<int>(input.size()), nullptr,
                                      0, nullptr, nullptr)};
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                      result.data(), size, nullptr, nullptr);
  return result;
#else
  static_assert(sizeof(wchar_t) >= 4,
                "a wide character must hold an entire codepoint");
  std::size_t size{0};
  for (const auto character : input) {
    size += utf8_codepoint_byte_count(static_cast<char32_t>(character));
  }

  std::string result;
  result.reserve(size);
  for (const auto character : input) {
    codepoint_to_utf8(static_cast<char32_t>(character), result);
  }

  return result;
#endif
}

auto combining_class(const char32_t codepoint) noexcept -> std::uint8_t {
  if (codepoint > 0x10FFFF) {
    return 0;
  }
  const std::size_t page{COMBINING_CLASS_STAGE1[codepoint >> 10U]};
  return COMBINING_CLASS_STAGE2[(page << 10U) | (codepoint & 0x3FFU)];
}

auto joining_type(const char32_t codepoint) noexcept -> JoiningType {
  if (codepoint > 0x10FFFF) {
    return JoiningType::NonJoining;
  }
  const std::size_t page{JOINING_TYPE_STAGE1[codepoint >> 10U]};
  return static_cast<JoiningType>(
      JOINING_TYPE_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]);
}

auto bidi_class(const char32_t codepoint) noexcept -> BidiClass {
  if (codepoint > 0x10FFFF) {
    return BidiClass::LeftToRight;
  }
  const std::size_t page{BIDI_CLASS_STAGE1[codepoint >> 10U]};
  return static_cast<BidiClass>(
      BIDI_CLASS_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]);
}

auto script(const char32_t codepoint) noexcept -> UnicodeScript {
  if (codepoint > 0x10FFFF) {
    return UnicodeScript::Unknown;
  }
  const std::size_t page{UNICODE_SCRIPT_STAGE1[codepoint >> 10U]};
  return static_cast<UnicodeScript>(
      UNICODE_SCRIPT_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]);
}

auto is_combining_mark(const char32_t codepoint) noexcept -> bool {
  if (codepoint > 0x10FFFF) {
    return false;
  }
  const std::size_t page{IS_COMBINING_MARK_STAGE1[codepoint >> 10U]};
  return IS_COMBINING_MARK_STAGE2[(page << 10U) | (codepoint & 0x3FFU)] != 0;
}

auto nfc_quick_check(const char32_t codepoint) noexcept -> NFCQuickCheck {
  if (codepoint > 0x10FFFF) {
    return NFCQuickCheck::Yes;
  }
  const std::size_t page{NFC_QUICK_CHECK_STAGE1[codepoint >> 10U]};
  return static_cast<NFCQuickCheck>(
      NFC_QUICK_CHECK_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]);
}

auto canonical_decomposition(const char32_t codepoint) noexcept
    -> std::u32string_view {
  if (codepoint > 0x10FFFF) {
    return {};
  }
  const std::size_t page{CANONICAL_DECOMPOSITION_STAGE1[codepoint >> 10U]};
  const std::uint16_t packed{
      CANONICAL_DECOMPOSITION_STAGE2[(page << 10U) | (codepoint & 0x3FFU)]};
  const auto length{static_cast<std::size_t>(packed >> 14U)};
  const auto offset{static_cast<std::size_t>(packed & 0x3FFFU)};
  return std::u32string_view{CANONICAL_DECOMPOSITION_BLOB + offset, length};
}

auto canonical_composition(const char32_t starter,
                           const char32_t combining) noexcept
    -> std::optional<char32_t> {
  std::size_t low{0};
  std::size_t high{sizeof(CANONICAL_COMPOSITIONS) /
                   sizeof(CANONICAL_COMPOSITIONS[0])};
  while (low < high) {
    const std::size_t middle{low + ((high - low) >> 1U)};
    const auto &entry{CANONICAL_COMPOSITIONS[middle]};
    if (entry.starter < starter ||
        (entry.starter == starter && entry.combining < combining)) {
      low = middle + 1;
    } else {
      high = middle;
    }
  }
  const auto count{sizeof(CANONICAL_COMPOSITIONS) /
                   sizeof(CANONICAL_COMPOSITIONS[0])};
  if (low < count && CANONICAL_COMPOSITIONS[low].starter == starter &&
      CANONICAL_COMPOSITIONS[low].combining == combining) {
    return CANONICAL_COMPOSITIONS[low].composed;
  }
  return std::nullopt;
}

} // namespace sourcemeta::core
