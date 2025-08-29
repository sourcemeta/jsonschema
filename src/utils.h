#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>

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

struct InputJSON {
  std::filesystem::path first;
  sourcemeta::core::JSON second;
  sourcemeta::core::PointerPositionTracker positions;
  auto operator<(const InputJSON &other) const noexcept -> bool {
    return this->first < other.first;
  }
};

auto for_each_json(const std::vector<std::string_view> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<InputJSON>;

auto print(const sourcemeta::blaze::SimpleOutput &output, std::ostream &stream)
    -> void;

auto print_annotations(const sourcemeta::blaze::SimpleOutput &output,
                       const sourcemeta::core::Options &options,
                       std::ostream &stream) -> void;

auto print(const sourcemeta::blaze::TraceOutput &output, std::ostream &stream)
    -> void;

auto resolver(const sourcemeta::core::Options &options, const bool remote,
              const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::SchemaResolver;

auto log_verbose(const sourcemeta::core::Options &options) -> std::ostream &;

auto parse_extensions(const sourcemeta::core::Options &options)
    -> std::set<std::string>;

auto parse_ignore(const sourcemeta::core::Options &options)
    -> std::set<std::filesystem::path>;

auto default_dialect(const sourcemeta::core::Options &options)
    -> std::optional<std::string>;

} // namespace sourcemeta::jsonschema::cli

#endif
