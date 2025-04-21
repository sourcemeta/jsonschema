#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>

#include <filesystem> // std::filesystem
#include <map>        // std::map
#include <ostream>    // std::ostream
#include <set>        // std::set
#include <span>       // std::span
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector

namespace sourcemeta::jsonschema::cli {

auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>>;

auto for_each_json_or_yaml(
    const std::vector<std::string> &arguments,
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::vector<std::pair<std::filesystem::path, sourcemeta::core::JSON>>;

auto print(const sourcemeta::blaze::SimpleOutput &output, std::ostream &stream)
    -> void;

auto print_annotations(
    const sourcemeta::blaze::SimpleOutput &output,
    const std::map<std::string, std::vector<std::string>> &options,
    std::ostream &stream) -> void;

auto print(const sourcemeta::blaze::TraceOutput &output, std::ostream &stream)
    -> void;

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream &;

auto log_error() -> std::ostream &;

auto log_warning() -> std::ostream &;

auto safe_weakly_canonical(const std::filesystem::path &input)
    -> std::filesystem::path;

auto looks_like_yaml(const std::filesystem::path &path) -> bool;

auto read_yaml_or_json(const std::filesystem::path &path)
    -> sourcemeta::core::JSON;

auto infer_resolver(
    const std::map<std::string, std::vector<std::string>> &options,
    const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::SchemaResolver;

auto infer_default_dialect(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::optional<std::string>;

} // namespace sourcemeta::jsonschema::cli

#endif
