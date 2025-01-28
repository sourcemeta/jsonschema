#ifndef SOURCEMETA_JSONTOOLKIT_YAML_H_
#define SOURCEMETA_JSONTOOLKIT_YAML_H_

#ifndef SOURCEMETA_JSONTOOLKIT_YAML_EXPORT
#include <sourcemeta/jsontoolkit/yaml_export.h>
#endif

#include <sourcemeta/jsontoolkit/yaml_error.h>

#include <sourcemeta/jsontoolkit/json.h>

#include <filesystem> // std::filesystem

/// @defgroup yaml YAML
/// @brief A YAML compatibility library based on `libyaml`.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/yaml.h>
/// ```

namespace sourcemeta::jsontoolkit {

/// @ingroup yaml
///
/// Create a JSON document from a C++ standard input stream that represents a
/// YAML document. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/yaml.h>
///
/// #include <iostream>
///
/// const std::string input{"hello: world"};
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::from_yaml(input);
/// sourcemeta::jsontoolkit::prettify(document, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_JSONTOOLKIT_YAML_EXPORT
auto from_yaml(const JSON::String &input) -> JSON;

/// @ingroup yaml
///
/// Read a JSON document from a file location that represents a YAML file. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/yaml.h>
///
/// #include <iostream>
/// #include <filesystem>
///
/// const std::filesystem::path path{"test.yaml"};
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::from_yaml(path);
/// sourcemeta::jsontoolkit::prettify(document, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_JSONTOOLKIT_YAML_EXPORT
auto from_yaml(const std::filesystem::path &path) -> JSON;

} // namespace sourcemeta::jsontoolkit

#endif
