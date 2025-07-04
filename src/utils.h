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

template <typename T> class FileError : public T {
public:
  template <typename... Args>
  FileError(std::filesystem::path path, Args &&...args)
      : T{std::forward<Args>(args)...}, path_{std::move(path)} {
    assert(std::filesystem::exists(this->path_));
  }

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return path_;
  }

private:
  std::filesystem::path path_;
};

auto read_file(const std::filesystem::path &path) -> sourcemeta::core::JSON;

auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>>;

auto for_each_json(const std::vector<std::string> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<std::pair<std::filesystem::path, sourcemeta::core::JSON>>;

auto print(const sourcemeta::blaze::SimpleOutput &output, std::ostream &stream)
    -> void;

auto print_annotations(
    const sourcemeta::blaze::SimpleOutput &output,
    const std::map<std::string, std::vector<std::string>> &options,
    std::ostream &stream) -> void;

auto print(const sourcemeta::blaze::TraceOutput &output, std::ostream &stream)
    -> void;

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote,
              const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::SchemaResolver;

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream &;

auto parse_extensions(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::string>;

auto parse_ignore(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::filesystem::path>;

auto safe_weakly_canonical(const std::filesystem::path &input)
    -> std::filesystem::path;

auto default_dialect(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::optional<std::string>;

} // namespace sourcemeta::jsonschema::cli

#endif
