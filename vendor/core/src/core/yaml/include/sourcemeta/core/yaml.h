#ifndef SOURCEMETA_CORE_YAML_H_
#define SOURCEMETA_CORE_YAML_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <sourcemeta/core/yaml_error.h>

#include <sourcemeta/core/json.h>

#include <filesystem> // std::filesystem

/// @defgroup yaml YAML
/// @brief A YAML compatibility library based on `libyaml`.
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
auto read_yaml(const std::filesystem::path &path) -> JSON;

} // namespace sourcemeta::core

#endif
