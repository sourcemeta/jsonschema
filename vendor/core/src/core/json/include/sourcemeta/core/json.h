#ifndef SOURCEMETA_CORE_JSON_H_
#define SOURCEMETA_CORE_JSON_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/json_auto.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/preprocessor.h>

#include <cstdint>    // std::uint64_t
#include <filesystem> // std::filesystem
#include <format> // std::formatter, std::format_context, std::format_parse_context, std::format_to
#include <fstream>          // std::basic_ifstream
#include <initializer_list> // std::initializer_list
#include <istream>          // std::basic_istream
#include <ostream>          // std::basic_ostream
#include <sstream>          // std::ostringstream
#include <string>           // std::basic_string

/// @defgroup json JSON
/// @brief A full-blown ECMA-404 implementation with read, write, and iterators
/// support.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// ```

namespace sourcemeta::core {

/// @ingroup json
///
/// Create a JSON document from a C++ standard input stream. For example, a JSON
/// document that represents an array can be parsed as follows:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ 1, 2, 3 ]"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
/// assert(document.is_array());
/// ```
///
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> JSON;

/// @ingroup json
///
/// Create a JSON document from a JSON string. For example, a JSON document that
/// represents an array can be parsed as follows:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
/// assert(document.is_array());
/// ```
///
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input)
    -> JSON;

/// @ingroup json
///
/// Create a JSON document from a C++ standard input stream, passing your own
/// `line` and `column` read/write position indicators. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ 1, 2, 3 ]"};
/// std::uint64_t line{1};
/// std::uint64_t column{0};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream, line, column);
/// assert(document.is_array());
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                std::uint64_t &line, std::uint64_t &column) -> JSON;

/// @ingroup json
///
/// Create a JSON document from a JSON string, passing your own
/// `line` and `column` read/write position indicators. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// std::uint64_t line{1};
/// std::uint64_t column{0};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]", line, column);
/// assert(document.is_array());
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                std::uint64_t &line, std::uint64_t &column) -> JSON;

/// @ingroup json
///
/// A convenience function to create a JSON document from a file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::read_json("/tmp/foo.json");
/// sourcemeta::core::stringify(document, std::cout);
/// std::cout << std::endl;
/// ```
///
/// If parsing fails, sourcemeta::core::JSONFileParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto read_json(const std::filesystem::path &path) -> JSON;

/// @ingroup json
///
/// Parse a JSON document from a C++ standard input stream into an existing
/// JSON value, invoking the given callback during parsing. The result is
/// constructed directly into the given reference rather than returned by value
/// to ensure that references passed through the parse callback (such as object
/// property names) remain valid after parsing completes.
///
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                JSON &output, const JSON::ParseCallback &callback) -> void;

/// @ingroup json
///
/// Parse a JSON document from a JSON string into an existing JSON value,
/// invoking the given callback during parsing. The result is constructed
/// directly into the given reference rather than returned by value to ensure
/// that references passed through the parse callback (such as object property
/// names) remain valid after parsing completes.
///
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                JSON &output, const JSON::ParseCallback &callback) -> void;

/// @ingroup json
///
/// Parse a JSON document from a C++ standard input stream into an existing
/// JSON value, passing your own `line` and `column` read/write position
/// indicators and invoking the given callback during parsing. The result is
/// constructed directly into the given reference rather than returned by value
/// to ensure that references passed through the parse callback (such as object
/// property names) remain valid after parsing completes.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                std::uint64_t &line, std::uint64_t &column, JSON &output,
                const JSON::ParseCallback &callback) -> void;

/// @ingroup json
///
/// Parse a JSON document from a JSON string into an existing JSON value,
/// passing your own `line` and `column` read/write position indicators and
/// invoking the given callback during parsing. The result is constructed
/// directly into the given reference rather than returned by value to ensure
/// that references passed through the parse callback (such as object property
/// names) remain valid after parsing completes.
SOURCEMETA_CORE_JSON_EXPORT
auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                std::uint64_t &line, std::uint64_t &column, JSON &output,
                const JSON::ParseCallback &callback) -> void;

/// @ingroup json
///
/// A convenience function to parse a JSON document from a file into an existing
/// JSON value, invoking the given callback during parsing. The result is
/// constructed directly into the given reference rather than returned by value
/// to ensure that references passed through the parse callback (such as object
/// property names) remain valid after parsing completes.
///
/// If parsing fails, sourcemeta::core::JSONFileParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto read_json(const std::filesystem::path &path, JSON &output,
               const JSON::ParseCallback &callback) -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// compact mode. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// sourcemeta::core::stringify(document, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// pretty mode. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// sourcemeta::core::prettify(document, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t spaces = 2) -> void;

/// @ingroup json
///
/// Encode the input JSON document into a given standard output stream.
/// The JSON document is stringified or prettified depending on the
/// presence of the `NDEBUG` define (for debugging purposes). For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// stream << document;
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON &document)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> &;

/// @ingroup json
///
/// Encode the input JSON type as a string into a given standard output stream.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// std::ostringstream stream;
/// stream << sourcemeta::core::JSON::Type::String;
/// // Will print "string"
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::Type type)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> &;

/// @ingroup json
///
/// Create a JSON type set from an initializer list of types. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
///
/// const auto types = sourcemeta::core::make_set(
///     {sourcemeta::core::JSON::Type::Object,
///      sourcemeta::core::JSON::Type::Array});
/// ```
SOURCEMETA_FORCEINLINE inline auto
make_set(std::initializer_list<JSON::Type> types) -> JSON::TypeSet {
  JSON::TypeSet result;
  for (const auto type : types) {
    result.set(static_cast<std::size_t>(type));
  }
  return result;
}

} // namespace sourcemeta::core

template <> struct std::formatter<sourcemeta::core::JSON> {
  constexpr auto parse(std::format_parse_context &context)
      -> decltype(context.begin()) {
    return context.begin();
  }

  auto format(const sourcemeta::core::JSON &value,
              std::format_context &context) const -> decltype(context.out()) {
    std::ostringstream stream;
    stream << value;
    return std::format_to(context.out(), "{}", stream.str());
  }
};

template <> struct std::formatter<sourcemeta::core::JSON::Type> {
  constexpr auto parse(std::format_parse_context &context)
      -> decltype(context.begin()) {
    return context.begin();
  }

  auto format(const sourcemeta::core::JSON::Type value,
              std::format_context &context) const -> decltype(context.out()) {
    std::ostringstream stream;
    stream << value;
    return std::format_to(context.out(), "{}", stream.str());
  }
};

#endif
