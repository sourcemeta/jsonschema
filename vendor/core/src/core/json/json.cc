#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>

#include "parser.h"
#include "stringify.h"

#include <cassert>      // assert
#include <cstdint>      // std::uint64_t
#include <filesystem>   // std::filesystem
#include <fstream>      // std::ifstream
#include <istream>      // std::basic_istream
#include <ostream>      // std::basic_ostream
#include <system_error> // std::make_error_code, std::errc

namespace sourcemeta::core {

auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                std::uint64_t &line, std::uint64_t &column,
                const JSON::ParseCallback &callback) -> JSON {
  return internal_parse_json(stream, line, column, callback);
}

auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                std::uint64_t &line, std::uint64_t &column,
                const JSON::ParseCallback &callback) -> JSON {
  return internal_parse_json(input, line, column, callback);
}

auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback) -> JSON {
  std::uint64_t line{1};
  std::uint64_t column{0};
  return parse_json(stream, line, column, callback);
}

auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                const JSON::ParseCallback &callback) -> JSON {
  std::uint64_t line{1};
  std::uint64_t column{0};
  return parse_json(input, line, column, callback);
}

auto read_file(const std::filesystem::path &path)
    -> std::basic_ifstream<JSON::Char, JSON::CharTraits> {
  if (std::filesystem::is_directory(path)) {
    throw std::filesystem::filesystem_error(
        "Cannot parse a directory as JSON", path,
        std::make_error_code(std::errc::is_a_directory));
  }

  std::ifstream stream{
      // On Linux, FIFO files (like /dev/fd/XX due to process substitution)
      // cannot be
      // made canonical
      // See https://github.com/sourcemeta/jsonschema/issues/252
      std::filesystem::is_fifo(path) ? path : std::filesystem::canonical(path)};
  stream.exceptions(std::ifstream::badbit);
  assert(!stream.fail());
  assert(stream.is_open());
  return stream;
}

auto read_json(const std::filesystem::path &path,
               const JSON::ParseCallback &callback) -> JSON {
  auto stream{read_file(path)};
  try {
    return parse_json(stream, callback);
  } catch (const JSONParseIntegerLimitError &error) {
    // For producing better error messages
    throw JSONFileParseError(path, error);
  } catch (const JSONParseError &error) {
    // For producing better error messages
    throw JSONFileParseError(path, error);
  }
}

auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<std::allocator>(document, stream, nullptr);
}

auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  prettify<std::allocator>(document, stream, nullptr);
}

auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
               const JSON::KeyComparison &compare) -> void {
  stringify<std::allocator>(document, stream, compare);
}

auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const JSON::KeyComparison &compare) -> void {
  prettify<std::allocator>(document, stream, compare);
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
