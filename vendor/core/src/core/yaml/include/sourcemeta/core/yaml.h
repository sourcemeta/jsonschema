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

#include <filesystem> // std::filesystem
#include <istream>    // std::basic_istream
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
/// YAML document. For example:
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
auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

/// @ingroup yaml
///
/// Create a JSON document from a C++ standard input stream that represents a
/// YAML document. For example:
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
auto parse_yaml(const JSON::String &input,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

/// @ingroup yaml
///
/// Read a JSON document from a file location that represents a YAML file. For
/// example:
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
auto read_yaml(const std::filesystem::path &path,
               const JSON::ParseCallback &callback = nullptr) -> JSON;

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
auto read_yaml_or_json(const std::filesystem::path &path,
                       const JSON::ParseCallback &callback = nullptr) -> JSON;

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
auto parse_yaml(const JSON::String &input, YAMLRoundTrip &roundtrip,
                const JSON::ParseCallback &callback = nullptr) -> JSON;

/// @ingroup yaml
///
/// Stringify a JSON document as YAML, using round-trip metadata collected
/// during parsing to preserve the original formatting. For example:
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
SOURCEMETA_CORE_YAML_EXPORT
auto stringify_yaml(const JSON &document,
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                    const YAMLRoundTrip &roundtrip) -> void;

/// @ingroup yaml
///
/// Stringify a JSON document as YAML. For example:
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
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

} // namespace sourcemeta::core

#endif
