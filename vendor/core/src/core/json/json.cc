#include <sourcemeta/core/io.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>

#include "construct.h"
#include "parser.h"
#include "stringify.h"

#include <cassert>     // assert
#include <cstdint>     // std::uint64_t
#include <filesystem>  // std::filesystem
#include <istream>     // std::basic_istream
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <ostream>     // std::basic_ostream
#include <type_traits> // std::conditional_t
#include <utility>     // std::cmp_greater
#include <vector>      // std::vector

namespace sourcemeta::core {

template <bool should_throw>
static auto internal_parse_json(const char *&cursor, const char *end,
                                std::uint64_t &line, std::uint64_t &column,
                                const JSON::ParseCallback &callback,
                                const bool track_positions, JSON &output)
    -> std::conditional_t<should_throw, void, bool> {
  const char *buffer_start{cursor};
  // Tape entries address the input with 32-bit offsets and lengths, so a larger
  // input cannot be represented without truncation
  if (std::cmp_greater(end - cursor,
                       std::numeric_limits<std::uint32_t>::max())) {
    if constexpr (should_throw) {
      throw JSONParseError(line, column);
    } else {
      return false;
    }
  }

  std::vector<TapeEntry> tape;
  tape.reserve(static_cast<std::size_t>(end - cursor) / 8);

  if constexpr (should_throw) {
    if (callback || track_positions) {
      scan_json<true>(cursor, end, buffer_start, line, column, tape);
    } else {
      // Re-scan with position tracking on failure for a precise error message
      try {
        scan_json<false>(cursor, end, buffer_start, line, column, tape);
      } catch (const JSONParseError &) {
        cursor = buffer_start;
        tape.clear();
        line = 1;
        column = 0;
        scan_json<true>(cursor, end, buffer_start, line, column, tape);
      }
    }
    construct_json(buffer_start, tape, callback, output);
  } else {
    // Both the scanning and the construction phases signal failure by throwing,
    // so a single boundary around them reports either as no value
    try {
      if (callback || track_positions) {
        scan_json<true>(cursor, end, buffer_start, line, column, tape);
      } else {
        scan_json<false>(cursor, end, buffer_start, line, column, tape);
      }
      construct_json(buffer_start, tape, callback, output);
    } catch (const JSONParseError &) {
      return false;
    }
    return true;
  }
}

static auto internal_parse_json(const char *&cursor, const char *end,
                                std::uint64_t &line, std::uint64_t &column,
                                const bool track_positions) -> JSON {
  JSON output{nullptr};
  internal_parse_json<true>(cursor, end, line, column, nullptr, track_positions,
                            output);
  return output;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                std::uint64_t &line, std::uint64_t &column) -> JSON {
  const auto start_position{stream.tellg()};
  const auto input{read_to_string(stream)};
  const char *cursor{input.data()};
  const char *end{input.data() + input.size()};
  auto result{internal_parse_json(cursor, end, line, column, true)};
  if (start_position != static_cast<std::streampos>(-1)) {
    const auto consumed{static_cast<std::streamoff>(cursor - input.data())};
    stream.clear();
    stream.seekg(start_position + consumed);
  }

  return result;
}

auto parse_json(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> input,
    std::uint64_t &line, std::uint64_t &column) -> JSON {
  const char *cursor{input.empty() ? "" : input.data()};
  return internal_parse_json(cursor, cursor + input.size(), line, column, true);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> JSON {
  const auto start_position{stream.tellg()};
  const auto input{read_to_string(stream)};
  const char *cursor{input.data()};
  const char *end{input.data() + input.size()};
  std::uint64_t line{1};
  std::uint64_t column{0};
  auto result{internal_parse_json(cursor, end, line, column, false)};
  if (start_position != static_cast<std::streampos>(-1)) {
    const auto consumed{static_cast<std::streamoff>(cursor - input.data())};
    stream.clear();
    stream.seekg(start_position + consumed);
  }
  return result;
}

auto parse_json(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> input) -> JSON {
  std::uint64_t line{1};
  std::uint64_t column{0};
  const char *cursor{input.empty() ? "" : input.data()};
  return internal_parse_json(cursor, cursor + input.size(), line, column,
                             false);
}

auto try_parse_json(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> input)
    -> std::optional<JSON> {
  std::uint64_t line{1};
  std::uint64_t column{0};
  const char *cursor{input.empty() ? "" : input.data()};
  JSON output{nullptr};
  if (internal_parse_json<false>(cursor, cursor + input.size(), line, column,
                                 nullptr, false, output)) {
    return output;
  }

  return std::nullopt;
}

auto read_json(const std::filesystem::path &path) -> JSON {
  try {
    return parse_json(read_file_to_string(path));
  } catch (const JSONParseError &error) {
    // For producing better error messages
    throw JSONFileParseError(path, error);
  }
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                std::uint64_t &line, std::uint64_t &column, JSON &output,
                const JSON::ParseCallback &callback) -> void {
  const auto start_position{stream.tellg()};
  const auto input{read_to_string(stream)};
  const char *cursor{input.data()};
  const char *end{input.data() + input.size()};
  internal_parse_json<true>(cursor, end, line, column, callback, true, output);
  if (start_position != static_cast<std::streampos>(-1)) {
    const auto consumed{static_cast<std::streamoff>(cursor - input.data())};
    stream.clear();
    stream.seekg(start_position + consumed);
  }
}

auto parse_json(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> input,
    std::uint64_t &line, std::uint64_t &column, JSON &output,
    const JSON::ParseCallback &callback) -> void {
  const char *cursor{input.empty() ? "" : input.data()};
  internal_parse_json<true>(cursor, cursor + input.size(), line, column,
                            callback, true, output);
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                JSON &output, const JSON::ParseCallback &callback) -> void {
  const auto start_position{stream.tellg()};
  const auto input{read_to_string(stream)};
  const char *cursor{input.data()};
  const char *end{input.data() + input.size()};
  std::uint64_t line{1};
  std::uint64_t column{0};
  internal_parse_json<true>(cursor, end, line, column, callback, false, output);
  if (start_position != static_cast<std::streampos>(-1)) {
    const auto consumed{static_cast<std::streamoff>(cursor - input.data())};
    stream.clear();
    stream.seekg(start_position + consumed);
  }
}

auto parse_json(
    const std::basic_string_view<JSON::Char, JSON::CharTraits> input,
    JSON &output, const JSON::ParseCallback &callback) -> void {
  std::uint64_t line{1};
  std::uint64_t column{0};
  const char *cursor{input.empty() ? "" : input.data()};
  internal_parse_json<true>(cursor, cursor + input.size(), line, column,
                            callback, false, output);
}

auto read_json(const std::filesystem::path &path, JSON &output,
               const JSON::ParseCallback &callback) -> void {
  try {
    parse_json(read_file_to_string(path), output, callback);
  } catch (const JSONParseError &error) {
    // For producing better error messages
    throw JSONFileParseError(path, error);
  }
}

auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<std::allocator>(document, stream);
}

auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t spaces) -> void {
  prettify<std::allocator>(document, stream, 0, spaces);
}

auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON &document)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> & {
#ifdef NDEBUG
  stringify(document, stream);
#else
  prettify(document, stream);
#endif
  return stream;
}

auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::Type type)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> & {
  switch (type) {
    case sourcemeta::core::JSON::Type::Null:
      return stream << "null";
    case sourcemeta::core::JSON::Type::Boolean:
      return stream << "boolean";
    case sourcemeta::core::JSON::Type::Integer:
      return stream << "integer";
    case sourcemeta::core::JSON::Type::Real:
      return stream << "real";
    case sourcemeta::core::JSON::Type::Decimal:
      return stream << "decimal";
    case sourcemeta::core::JSON::Type::String:
      return stream << "string";
    case sourcemeta::core::JSON::Type::Array:
      return stream << "array";
    case sourcemeta::core::JSON::Type::Object:
      return stream << "object";
    default:
      // Should never happen, but some compilers are not happy without this
      assert(false);
      return stream;
  }
}

} // namespace sourcemeta::core
