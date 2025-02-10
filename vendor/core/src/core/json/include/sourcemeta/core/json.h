#ifndef SOURCEMETA_CORE_JSON_H_
#define SOURCEMETA_CORE_JSON_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_hash.h>
#include <sourcemeta/core/json_value.h>

#include <cstdint>    // std::uint64_t
#include <filesystem> // std::filesystem
#include <fstream>    // std::basic_ifstream
#include <istream>    // std::basic_istream
#include <ostream>    // std::basic_ostream
#include <string>     // std::basic_string

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
auto parse_json(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

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
auto parse_json(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

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
                std::uint64_t &line, std::uint64_t &column,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

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
                std::uint64_t &line, std::uint64_t &column,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

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
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto read_json(const std::filesystem::path &path) -> JSON;

// TODO: Move this function to a system integration component, as it
// is not JSON specific

/// @ingroup json
///
/// A convenience function to read a document from a file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
/// #include <iostream>
///
/// auto stream = sourcemeta::core::read_file("/tmp/foo.json");
/// const auto document = sourcemeta::core::parse_json(stream);
/// sourcemeta::core::stringify(document, std::cout);
/// std::cout << std::endl;
/// ```
///
/// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
SOURCEMETA_CORE_JSON_EXPORT
auto read_file(const std::filesystem::path &path)
    -> std::basic_ifstream<JSON::Char, JSON::CharTraits>;

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
/// pretty mode, indenting the output using 4 spaces. For example:
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
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream) -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// compact mode, sorting object properties on a specific criteria. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// auto key_compare(const sourcemeta::core::JSON::String &left,
///                  const sourcemeta::core::JSON::String &right)
///   -> bool {
///   return left < right;
/// }
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2, \"baz\": 3 }");
/// std::ostringstream stream;
/// sourcemeta::core::stringify(document, stream, key_compare);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
               const JSON::KeyComparison &compare) -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// pretty mode, indenting the output using 4 spaces and sorting object
/// properties on a specific criteria. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// auto key_compare(const sourcemeta::core::JSON::String &left,
///                  const sourcemeta::core::JSON::String &right)
///   -> bool {
///   return left < right;
/// }
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("{ \"foo\": 1, \"bar\": 2, \"baz\": 3 }");
/// std::ostringstream stream;
/// sourcemeta::core::prettify(document, stream, key_compare);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSON_EXPORT
auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const JSON::KeyComparison &compare) -> void;

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

} // namespace sourcemeta::core

#endif
