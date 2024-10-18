#ifndef SOURCEMETA_JSONTOOLKIT_JSON_H_
#define SOURCEMETA_JSONTOOLKIT_JSON_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
#include <sourcemeta/jsontoolkit/json_export.h>
#endif

#include <sourcemeta/jsontoolkit/json_error.h>
#include <sourcemeta/jsontoolkit/json_value.h>

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
/// #include <sourcemeta/jsontoolkit/json.h>
/// ```

namespace sourcemeta::jsontoolkit {

/// @ingroup json
/// Create a JSON document from a C++ standard input stream. For example, a JSON
/// document that represents an array can be parsed as follows:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ 1, 2, 3 ]"};
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(stream);
/// assert(document.is_array());
/// ```
///
/// If parsing fails, sourcemeta::jsontoolkit::ParseError will be thrown.
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto parse(std::basic_istream<JSON::Char, JSON::CharTraits> &stream) -> JSON;

/// @ingroup json
///
/// Create a JSON document from a JSON string. For example, a JSON document that
/// represents an array can be parsed as follows:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("[ 1, 2, 3 ]");
/// assert(document.is_array());
/// ```
///
/// If parsing fails, sourcemeta::jsontoolkit::ParseError will be thrown.
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto parse(const std::basic_string<JSON::Char, JSON::CharTraits> &input)
    -> JSON;

/// @ingroup json
/// Create a JSON document from a C++ standard input stream, passing your own
/// `line` and `column` read/write position indicators. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ 1, 2, 3 ]"};
/// std::uint64_t line{1};
/// std::uint64_t column{0};
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(stream, line, column);
/// assert(document.is_array());
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto parse(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
           std::uint64_t &line, std::uint64_t &column) -> JSON;

/// @ingroup json
/// Create a JSON document from a JSON string, passing your own
/// `line` and `column` read/write position indicators. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
///
/// std::uint64_t line{1};
/// std::uint64_t column{0};
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("[ 1, 2, 3 ]", line, column);
/// assert(document.is_array());
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto parse(const std::basic_string<JSON::Char, JSON::CharTraits> &input,
           std::uint64_t &line, std::uint64_t &column) -> JSON;

/// @ingroup json
///
/// A convenience function to read a document from a file. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
/// #include <iostream>
///
/// auto stream = sourcemeta::jsontoolkit::read_file("/tmp/foo.json");
/// const auto document = sourcemeta::jsontoolkit::parse(stream);
/// sourcemeta::jsontoolkit::stringify(document, std::cout);
/// std::cout << std::endl;
/// ```
///
/// If parsing fails, sourcemeta::jsontoolkit::ParseError will be thrown.
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto read_file(const std::filesystem::path &path)
    -> std::basic_ifstream<JSON::Char, JSON::CharTraits>;

/// @ingroup json
///
/// A convenience function to create a JSON document from a file. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <cassert>
/// #include <iostream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::from_file("/tmp/foo.json");
/// sourcemeta::jsontoolkit::stringify(document, std::cout);
/// std::cout << std::endl;
/// ```
///
/// If parsing fails, sourcemeta::jsontoolkit::ParseError will be thrown.
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto from_file(const std::filesystem::path &path) -> JSON;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// compact mode. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// sourcemeta::jsontoolkit::stringify(document, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// pretty mode, indenting the output using 4 spaces. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// sourcemeta::jsontoolkit::prettify(document, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream) -> void;

/// @ingroup json
/// A comparison function between object property keys.
/// See https://en.cppreference.com/w/cpp/named_req/Compare
using KeyComparison =
    std::function<bool(const JSON::String &, const JSON::String &)>;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// compact mode, sorting object properties on a specific criteria. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// auto key_compare(const sourcemeta::jsontoolkit::JSON::String &left,
///                  const sourcemeta::jsontoolkit::JSON::String &right)
///   -> bool {
///   return left < right;
/// }
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("{ \"foo\": 1, \"bar\": 2, \"baz\": 3 }");
/// std::ostringstream stream;
/// sourcemeta::jsontoolkit::stringify(document, stream, key_compare);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
               const KeyComparison &compare) -> void;

/// @ingroup json
///
/// Stringify the input JSON document into a given C++ standard output stream in
/// pretty mode, indenting the output using 4 spaces and sorting object
/// properties on a specific criteria. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// auto key_compare(const sourcemeta::jsontoolkit::JSON::String &left,
///                  const sourcemeta::jsontoolkit::JSON::String &right)
///   -> bool {
///   return left < right;
/// }
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("{ \"foo\": 1, \"bar\": 2, \"baz\": 3 }");
/// std::ostringstream stream;
/// sourcemeta::jsontoolkit::prettify(document, stream, key_compare);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const KeyComparison &compare) -> void;

/// @ingroup json
///
/// Encode the input JSON document into a given standard output stream.
/// The JSON document is stringified or prettified depending on the
/// presence of the `NDEBUG` define (for debugging purposes). For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse("[ 1, 2, 3 ]");
/// std::ostringstream stream;
/// stream << document;
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON &document)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> &;

/// @ingroup json
///
/// Encode the input JSON type as a string into a given standard output stream.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// std::ostringstream stream;
/// stream << sourcemeta::jsontoolkit::JSON::Type::String;
/// // Will print "string"
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSON_EXPORT
auto operator<<(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::Type type)
    -> std::basic_ostream<JSON::Char, JSON::CharTraits> &;

} // namespace sourcemeta::jsontoolkit

#endif
