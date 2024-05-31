#ifndef INTELLIGENCE_JSONSCHEMA_CLI_UTILS_H_
#define INTELLIGENCE_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <filesystem> // std::filesystem
#include <map>        // std::map
#include <ostream>    // std::ostream
#include <set>        // std::set
#include <span>       // std::span
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector

#define CLI_ENSURE(condition, message)                                         \
  if (!(condition)) {                                                          \
    std::cerr << message << "\n";                                              \
    return EXIT_FAILURE;                                                       \
  }

namespace intelligence::jsonschema::cli {

// TODO: Tweak this function to take:
// - The options that take values
// - The options that are aliases to other options
auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>>;

auto for_each_json(const std::vector<std::string> &arguments)
    -> std::vector<
        std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>;

auto pretty_evaluate_callback(
    bool result,
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &,
    const sourcemeta::jsontoolkit::Pointer &evaluate_path,
    const sourcemeta::jsontoolkit::Pointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &,
    const sourcemeta::jsontoolkit::JSON &) -> void;

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote = false)
    -> sourcemeta::jsontoolkit::SchemaResolver;

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream &;

} // namespace intelligence::jsonschema::cli

#endif
