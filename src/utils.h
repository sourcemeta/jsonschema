#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

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

auto for_each_json(const std::vector<std::string> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<
        std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>;

auto print(const sourcemeta::blaze::ErrorOutput &output, std::ostream &stream)
    -> void;

auto print(const sourcemeta::blaze::TraceOutput &output, std::ostream &stream)
    -> void;

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote = false)
    -> sourcemeta::jsontoolkit::SchemaResolver;

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream &;

auto parse_extensions(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::string>;

auto parse_ignore(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::filesystem::path>;

} // namespace sourcemeta::jsonschema::cli

#endif
