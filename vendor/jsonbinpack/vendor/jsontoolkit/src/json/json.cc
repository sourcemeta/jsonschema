#include "parser.h"
#include "stringify.h"

#include <sourcemeta/jsontoolkit/json.h>

#include <cassert>      // assert
#include <fstream>      // std::ifstream
#include <system_error> // std::make_error_code, std::errc

namespace sourcemeta::jsontoolkit {

auto parse(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
           std::uint64_t &line, std::uint64_t &column, const Callback &callback)
    -> JSON {
  return internal_parse(stream, line, column, callback);
}

auto parse(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
           std::uint64_t &line, std::uint64_t &column, const Callback &callback)
    -> JSON {
  return internal_parse(input, line, column, callback);
}

auto parse(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
           const Callback &callback) -> JSON {
  std::uint64_t line{1};
  std::uint64_t column{0};
  return parse(stream, line, column, callback);
}

auto parse(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
           const Callback &callback) -> JSON {
  std::uint64_t line{1};
  std::uint64_t column{0};
  return parse(input, line, column, callback);
}

auto read_file(const std::filesystem::path &path)
    -> std::basic_ifstream<JSON::Char, JSON::CharTraits> {
  if (std::filesystem::is_directory(path)) {
    throw std::filesystem::filesystem_error(
        "Cannot parse a directory as JSON", path,
        std::make_error_code(std::errc::is_a_directory));
  }

  std::ifstream stream{std::filesystem::canonical(path)};
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  assert(!stream.fail());
  assert(stream.is_open());
  return stream;
}

auto from_file(const std::filesystem::path &path) -> JSON {
  auto stream{read_file(path)};
  try {
    return parse(stream);
  } catch (const ParseError &error) {
    // For producing better error messages
    throw FileParseError(path, error);
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
               const KeyComparison &compare) -> void {
  stringify<std::allocator>(document, stream, compare);
}

auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const KeyComparison &compare) -> void {
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
    case sourcemeta::jsontoolkit::JSON::Type::Null:
      return stream << "null";
    case sourcemeta::jsontoolkit::JSON::Type::Boolean:
      return stream << "boolean";
    case sourcemeta::jsontoolkit::JSON::Type::Integer:
      return stream << "integer";
    case sourcemeta::jsontoolkit::JSON::Type::Real:
      return stream << "real";
    case sourcemeta::jsontoolkit::JSON::Type::String:
      return stream << "string";
    case sourcemeta::jsontoolkit::JSON::Type::Array:
      return stream << "array";
    case sourcemeta::jsontoolkit::JSON::Type::Object:
      return stream << "object";
    default:
      // Should never happen, but some compilers are not happy without this
      assert(false);
      return stream;
  }
}

} // namespace sourcemeta::jsontoolkit
