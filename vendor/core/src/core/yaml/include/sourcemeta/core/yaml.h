#ifndef SOURCEMETA_CORE_YAML_H_
#define SOURCEMETA_CORE_YAML_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <sourcemeta/core/json.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/yaml_error.h>
#include <sourcemeta/core/yaml_roundtrip.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>    // std::size_t
#include <filesystem> // std::filesystem
#include <istream>    // std::basic_istream
#include <optional>   // std::optional, std::nullopt
#include <ostream>    // std::basic_ostream

/// @defgroup yaml YAML
/// @brief A YAML parser that converts YAML to JSON.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/yaml.h>
/// ```

namespace sourcemeta::core {

/// @ingroup yaml
///
/// Create a JSON document from a C++ standard input stream that represents a
/// YAML document. The input must be UTF-8, optionally preceded by a UTF-8 byte
/// order mark, as UTF-16 and UTF-32 input are not supported. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"foo: bar"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_yaml(stream);
/// assert(document.is_object());
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> JSON;

/// @ingroup yaml
///
/// Create a JSON document from a C++ standard input stream that represents a
/// YAML document. The input must be UTF-8, optionally preceded by a UTF-8 byte
/// order mark, as UTF-16 and UTF-32 input are not supported. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
///
/// const std::string input{"hello: world"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_yaml(input);
/// sourcemeta::core::prettify(document, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(const JSON::String &input) -> JSON;

/// @ingroup yaml
///
/// Read a JSON document from a file location that represents a YAML file. The
/// file must be UTF-8, optionally preceded by a UTF-8 byte order mark, as
/// UTF-16 and UTF-32 input are not supported. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
/// #include <filesystem>
///
/// const std::filesystem::path path{"test.yaml"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::read_yaml(path);
/// sourcemeta::core::prettify(document, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml(const std::filesystem::path &path) -> JSON;

/// @ingroup yaml
///
/// Parse a YAML document from a C++ standard input stream into an existing
/// JSON value, invoking the given callback during parsing. The result is
/// constructed directly into the given reference rather than returned by value
/// to ensure that references passed through the parse callback remain valid
/// after parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                JSON &output, const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Parse a YAML string into an existing JSON value, invoking the given
/// callback during parsing. The result is constructed directly into the given
/// reference rather than returned by value to ensure that references passed
/// through the parse callback remain valid after parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(const JSON::String &input, JSON &output,
                const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Read a YAML file into an existing JSON value, invoking the given callback
/// during parsing. The result is constructed directly into the given reference
/// rather than returned by value to ensure that references passed through the
/// parse callback remain valid after parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml(const std::filesystem::path &path, JSON &output,
               const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Read a JSON document from a file location that represents a YAML file or a
/// JSON file. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
/// #include <filesystem>
///
/// const std::filesystem::path path{"test.yaml"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::read_yaml_or_json(path);
/// sourcemeta::core::prettify(document, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml_or_json(const std::filesystem::path &path) -> JSON;

/// @ingroup yaml
///
/// Read a JSON document from a file that represents YAML or JSON, constructing
/// into the given reference and invoking the callback during parsing. The
/// result is constructed directly into the given reference rather than returned
/// by value to ensure that references passed through the parse callback (such
/// as object property names) remain valid after parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml_or_json(const std::filesystem::path &path, JSON &output,
                       const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Create a JSON document from a YAML string, collecting round-trip metadata
/// to reproduce the original formatting. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// sourcemeta::core::YAMLRoundTrip roundtrip;
/// const std::string input{"hello: world"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_yaml(input, roundtrip);
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(const JSON::String &input, YAMLRoundTrip &roundtrip) -> JSON;

/// @ingroup yaml
///
/// Create a JSON document from a C++ standard input stream that represents a
/// YAML document, collecting round-trip metadata to reproduce the original
/// formatting. The stream is left just after the document that was read, so
/// that a stream holding several documents can be read one document at a
/// time. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <sstream>
///
/// std::istringstream stream{"hello: world\n---\nsecond: document\n"};
/// while (stream.peek() != std::char_traits<char>::eof()) {
///   sourcemeta::core::YAMLRoundTrip roundtrip;
///   const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_yaml(stream, roundtrip);
/// }
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                YAMLRoundTrip &roundtrip) -> JSON;

/// @ingroup yaml
///
/// Parse a YAML string with round-trip metadata into an existing JSON value,
/// invoking the given callback during parsing. The result is constructed
/// directly into the given reference rather than returned by value to ensure
/// that references passed through the parse callback remain valid after
/// parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(const JSON::String &input, YAMLRoundTrip &roundtrip,
                JSON &output, const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Parse a YAML document from a C++ standard input stream with round-trip
/// metadata into an existing JSON value, invoking the given callback during
/// parsing. The stream is left just after the document that was read, so that
/// a stream holding several documents can be read one document at a time. The
/// result is constructed directly into the given reference rather than
/// returned by value to ensure that references passed through the parse
/// callback remain valid after parsing completes.
SOURCEMETA_CORE_YAML_EXPORT
auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                YAMLRoundTrip &roundtrip, JSON &output,
                const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Read a JSON document from a file location that represents a YAML file,
/// collecting round-trip metadata to reproduce the original formatting. Unlike
/// the stream overload, the file must hold a single document, as a file that
/// carries more cannot be written back from one set of metadata. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
///
/// sourcemeta::core::YAMLRoundTrip roundtrip;
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::read_yaml("test.yaml", roundtrip);
/// sourcemeta::core::stringify_yaml(document, std::cout, roundtrip);
/// ```
///
/// If parsing fails, sourcemeta::core::YAMLFileParseError will be thrown.
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml(const std::filesystem::path &path, YAMLRoundTrip &roundtrip)
    -> JSON;

/// @ingroup yaml
///
/// Read a YAML file with round-trip metadata into an existing JSON value,
/// invoking the given callback during parsing. The file must hold a single
/// document. The result is constructed directly into the given reference
/// rather than returned by value to ensure that references passed through the
/// parse callback remain valid after parsing completes.
///
/// If parsing fails, sourcemeta::core::YAMLFileParseError will be thrown.
SOURCEMETA_CORE_YAML_EXPORT
auto read_yaml(const std::filesystem::path &path, YAMLRoundTrip &roundtrip,
               JSON &output, const JSON::ParseCallback &callback) -> void;

/// @ingroup yaml
///
/// Stringify a JSON document as YAML, using round-trip metadata collected
/// during parsing to preserve the original formatting. The document may be
/// modified in between, in which case the nodes that changed are written from
/// the document, keeping their original presentation style where that style
/// can still express them. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
///
/// sourcemeta::core::YAMLRoundTrip roundtrip;
/// const std::string input{"hello: world"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_yaml(input, roundtrip);
/// sourcemeta::core::stringify_yaml(document, std::cout, roundtrip);
/// ```
///
/// Each level of nesting is laid out with the width the document was written
/// with, unless an indentation is given, which overrides it. A width of zero
/// would run a nested collection into the one that holds it, so it is treated
/// as one.
SOURCEMETA_CORE_YAML_EXPORT
auto stringify_yaml(const JSON &document,
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                    const YAMLRoundTrip &roundtrip,
                    const std::optional<std::size_t> indentation = std::nullopt)
    -> void;

/// @ingroup yaml
///
/// Stringify a JSON document as YAML, laying out each level of nesting with
/// the given number of spaces. A width of zero would run a nested collection
/// into the one that holds it, so it is treated as one. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/yaml.h>
///
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({ "foo": "bar" })JSON");
/// sourcemeta::core::stringify_yaml(document, std::cout);
/// ```
SOURCEMETA_CORE_YAML_EXPORT
auto stringify_yaml(const JSON &document,
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                    const std::size_t indentation = 2) -> void;

} // namespace sourcemeta::core

#endif
