#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cpr/cpr.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>

#include <algorithm>  // std::any_of, std::none_of, std::sort
#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <fstream>    // std::ofstream
#include <iostream>   // std::cerr
#include <map>        // std::map
#include <optional>   // std::optional, std::nullopt
#include <ostream>    // std::ostream
#include <set>        // std::set
#include <span>       // std::span
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::runtime_error
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

inline auto log_verbose(const sourcemeta::core::Options &options)
    -> std::ostream & {
  if (options.contains("verbose")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

inline auto find_configuration(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  return sourcemeta::core::SchemaConfig::find(path);
}

inline auto read_configuration(
    const sourcemeta::core::Options &options,
    const std::optional<std::filesystem::path> &configuration_path)
    -> const std::optional<sourcemeta::core::SchemaConfig> & {
  using CacheKey = std::optional<std::filesystem::path>;
  static std::map<CacheKey, std::optional<sourcemeta::core::SchemaConfig>>
      configuration_cache;

  // Check if configuration is already cached for this path
  auto iterator{configuration_cache.find(configuration_path)};
  if (iterator != configuration_cache.end()) {
    return iterator->second;
  }

  // Compute and cache the configuration
  std::optional<sourcemeta::core::SchemaConfig> result{std::nullopt};
  if (configuration_path.has_value()) {
    log_verbose(options) << "Using configuration file: "
                         << sourcemeta::core::weakly_canonical(
                                configuration_path.value())
                                .string()
                         << "\n";
    try {
      result =
          sourcemeta::core::SchemaConfig::read_json(configuration_path.value());
    } catch (const sourcemeta::core::SchemaConfigParseError &error) {
      throw FileError<sourcemeta::core::SchemaConfigParseError>(
          configuration_path.value(), error);
    }
  }

  auto [inserted_iterator, inserted] =
      configuration_cache.emplace(configuration_path, std::move(result));
  return inserted_iterator->second;
}

inline auto parse_extensions(const sourcemeta::core::Options &options)
    -> std::set<std::string> {
  std::set<std::string> result;

  if (options.contains("extension")) {
    for (const auto &extension : options.at("extension")) {
      log_verbose(options) << "Using extension: " << extension << "\n";
      if (extension.starts_with('.')) {
        result.emplace(extension);
      } else {
        std::ostringstream normalised_extension;
        normalised_extension << '.' << extension;
        result.emplace(normalised_extension.str());
      }
    }
  }

  if (result.empty()) {
    result.insert({".json"});
    result.insert({".yaml"});
    result.insert({".yml"});
  }

  return result;
}

inline auto parse_ignore(const sourcemeta::core::Options &options)
    -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> result;

  if (options.contains("ignore")) {
    for (const auto &ignore : options.at("ignore")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  return result;
}

inline auto default_dialect(
    const sourcemeta::core::Options &options,
    const std::optional<sourcemeta::core::SchemaConfig> &configuration)
    -> std::optional<std::string> {
  if (options.contains("default-dialect")) {
    return std::string{options.at("default-dialect").front()};
  } else if (configuration.has_value()) {
    return configuration.value().default_dialect;
  }

  return std::nullopt;
}

inline auto parse_indentation(const sourcemeta::core::Options &options)
    -> std::size_t {
  if (options.contains("indentation")) {
    return std::stoull(std::string{options.at("indentation").front()});
  }

  return 2;
}

namespace {

inline auto
handle_json_entry(const std::filesystem::path &entry_path,
                  const std::set<std::filesystem::path> &blacklist,
                  const std::set<std::string> &extensions,
                  std::vector<sourcemeta::jsonschema::cli::InputJSON> &result)
    -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      auto canonical{sourcemeta::core::weakly_canonical(entry.path())};
      if (!std::filesystem::is_directory(entry) &&
          std::any_of(extensions.cbegin(), extensions.cend(),
                      [&canonical](const auto &extension) {
                        return canonical.string().ends_with(extension);
                      }) &&
          std::none_of(blacklist.cbegin(), blacklist.cend(),
                       [&canonical](const auto &prefix) {
                         return sourcemeta::core::starts_with(canonical,
                                                              prefix);
                       })) {
        if (std::filesystem::is_empty(canonical)) {
          continue;
        }

        sourcemeta::core::PointerPositionTracker positions;
        // TODO: Print a verbose message for what is getting parsed
        auto contents{sourcemeta::core::read_yaml_or_json(canonical,
                                                          std::ref(positions))};
        result.push_back(
            {std::move(canonical), std::move(contents), std::move(positions)});
      }
    }
  } else {
    const auto canonical{sourcemeta::core::weakly_canonical(entry_path)};
    if (!std::filesystem::exists(canonical)) {
      std::ostringstream error;
      error << "No such file or directory\n  " << canonical.string();
      throw std::runtime_error(error.str());
    }

    if (std::none_of(blacklist.cbegin(), blacklist.cend(),
                     [&canonical](const auto &prefix) {
                       return sourcemeta::core::starts_with(canonical, prefix);
                     })) {
      if (std::filesystem::is_empty(canonical)) {
        return;
      }

      sourcemeta::core::PointerPositionTracker positions;
      // TODO: Print a verbose message for what is getting parsed
      auto contents{
          sourcemeta::core::read_yaml_or_json(canonical, std::ref(positions))};
      result.push_back(
          {std::move(canonical), std::move(contents), std::move(positions)});
    }
  }
}

} // namespace

inline auto for_each_json(const std::vector<std::string_view> &arguments,
                          const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  const auto blacklist{parse_ignore(options)};
  const auto extensions{parse_extensions(options)};
  std::vector<InputJSON> result;

  if (arguments.empty()) {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    const auto &configuration{read_configuration(options, configuration_path)};
    handle_json_entry(configuration.has_value()
                          ? configuration.value().absolute_path
                          : current_path,
                      blacklist, extensions, result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, blacklist, extensions, result);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &left, const auto &right) { return left < right; });

  return result;
}

inline auto for_each_json(const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  return for_each_json(options.positional(), options);
}

inline auto print(const sourcemeta::blaze::SimpleOutput &output,
                  std::ostream &stream) -> void {
  stream << "error: Schema validation failure\n";
  output.stacktrace(stream, "  ");
}

inline auto print_annotations(const sourcemeta::blaze::SimpleOutput &output,
                              const sourcemeta::core::Options &options,
                              std::ostream &stream) -> void {
  if (options.contains("verbose")) {
    for (const auto &annotation : output.annotations()) {
      for (const auto &value : annotation.second) {
        stream << "annotation: ";
        sourcemeta::core::stringify(value, stream);
        stream << "\n  at instance location \"";
        sourcemeta::core::stringify(annotation.first.instance_location, stream);
        stream << "\"\n  at evaluate path \"";
        sourcemeta::core::stringify(annotation.first.evaluate_path, stream);
        stream << "\"\n";
      }
    }
  }
}

// TODO: Move this as an operator<< overload for TraceOutput in Blaze itself
inline auto print(const sourcemeta::blaze::TraceOutput &output,
                  std::ostream &stream) -> void {
  for (auto iterator = output.cbegin(); iterator != output.cend(); iterator++) {
    const auto &entry{*iterator};

    if (entry.evaluate_path.empty()) {
      continue;
    }

    switch (entry.type) {
      case sourcemeta::blaze::TraceOutput::EntryType::Push:
        stream << "-> (push) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Pass:
        stream << "<- (pass) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Fail:
        stream << "<- (fail) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Annotation:
        stream << "@- (annotation) ";
        break;
      default:
        assert(false);
        break;
    }

    stream << "\"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\" (" << entry.name << ")\n";

    if (entry.annotation.has_value()) {
      stream << "   value ";

      if (entry.annotation.value().is_object()) {
        sourcemeta::core::stringify(entry.annotation.value(), stream);
      } else {
        sourcemeta::core::prettify(entry.annotation.value(), stream,
                                   sourcemeta::core::schema_format_compare);
      }

      stream << "\n";
    }

    stream << "   at \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"\n";
    stream << "   at keyword location \"" << entry.keyword_location << "\"\n";

    if (entry.vocabulary.first) {
      stream << "   at vocabulary \""
             << entry.vocabulary.second.value_or("<unknown>") << "\"\n";
    }

    // To make it easier to read
    if (std::next(iterator) != output.cend()) {
      stream << "\n";
    }
  }
}

static inline auto fallback_resolver(const sourcemeta::core::Options &options,
                                     std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON> {
  auto official_result{sourcemeta::core::schema_official_resolver(identifier)};
  if (official_result.has_value()) {
    return official_result;
  }

  // If the URI is not an HTTP URL, then abort
  const sourcemeta::core::URI uri{std::string{identifier}};
  const auto maybe_scheme{uri.scheme()};
  if (uri.is_urn() || !maybe_scheme.has_value() ||
      (maybe_scheme.value() != "https" && maybe_scheme.value() != "http")) {
    return std::nullopt;
  }

  log_verbose(options) << "Resolving over HTTP: " << identifier << "\n";
  const cpr::Response response{
      cpr::Get(cpr::Url{identifier}, cpr::Redirect{true})};

  if (response.status_code != 200) {
    std::ostringstream error;
    error << "HTTP " << response.status_code << "\n  at " << identifier;
    throw std::runtime_error(error.str());
  }

  const auto content_type_iterator{response.header.find("content-type")};
  if (content_type_iterator != response.header.end() &&
      content_type_iterator->second.starts_with("text/yaml")) {
    return sourcemeta::core::parse_yaml(response.text);
  } else {
    return sourcemeta::core::parse_json(response.text);
  }
}

class CustomResolver {
public:
  CustomResolver(
      const sourcemeta::core::Options &options,
      const std::optional<sourcemeta::core::SchemaConfig> &configuration,
      const bool remote, const std::optional<std::string> &default_dialect)
      : options_{options}, configuration_{configuration}, remote_{remote} {
    if (options.contains("resolve")) {
      for (const auto &entry : for_each_json(options.at("resolve"), options)) {
        log_verbose(options)
            << "Detecting schema resources from file: " << entry.first.string()
            << "\n";
        const auto result =
            this->add(entry.second, default_dialect,
                      sourcemeta::core::URI::from_path(entry.first).recompose(),
                      [&options](const auto &identifier) {
                        log_verbose(options)
                            << "Importing schema into the resolution context: "
                            << identifier << "\n";
                      });
        if (!result) {
          std::cerr
              << "warning: No schema resources were imported from this file\n";
          std::cerr << "  at " << entry.first.string() << "\n";
          std::cerr << "Are you sure this schema sets any identifiers?\n";
        }
      }
    }
  }

  auto add(const sourcemeta::core::JSON &schema,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt,
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr) -> bool {
    assert(sourcemeta::core::is_schema(schema));

    // Registering the top-level schema is not enough. We need to check
    // and register every embedded schema resource too
    sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::References};
    frame.analyse(schema, sourcemeta::core::schema_official_walker, *this,
                  default_dialect, default_id);

    bool added_any_schema{false};
    for (const auto &[key, entry] : frame.locations()) {
      if (entry.type != sourcemeta::core::SchemaFrame::LocationType::Resource) {
        continue;
      }

      auto subschema{sourcemeta::core::get(schema, entry.pointer)};
      const auto subschema_vocabularies{frame.vocabularies(entry, *this)};

      // Given we might be resolving embedded resources, we fully
      // resolve their dialect and identifiers, otherwise the
      // consumer might have no idea what to do with them
      subschema.assign("$schema", sourcemeta::core::JSON{entry.dialect});
      sourcemeta::core::reidentify(subschema, key.second, entry.base_dialect);

      const auto result{this->schemas.emplace(key.second, subschema)};
      if (!result.second && result.first->second != schema) {
        std::ostringstream error;
        error << "Cannot register the same identifier twice: " << key.second;
        throw sourcemeta::core::SchemaError(error.str());
      }

      if (callback) {
        callback(key.second);
      }

      added_any_schema = true;
    }

    return added_any_schema;
  }

  auto operator()(std::string_view identifier) const
      -> std::optional<sourcemeta::core::JSON> {
    const std::string string_identifier{identifier};
    if (this->configuration_.has_value()) {
      const auto match{
          this->configuration_.value().resolve.find(string_identifier)};
      if (match != this->configuration_.value().resolve.cend()) {
        const sourcemeta::core::URI new_uri{match->second};
        if (new_uri.is_relative()) {
          const auto file_uri{sourcemeta::core::URI::from_path(
              this->configuration_.value().absolute_path / new_uri.to_path())};
          const auto result{file_uri.recompose()};
          log_verbose(this->options_)
              << "Resolving " << identifier << " as " << result
              << " given the configuration file\n";
          return this->operator()(result);
        } else {
          log_verbose(this->options_)
              << "Resolving " << identifier << " as " << match->second
              << " given the configuration file\n";
          return this->operator()(match->second);
        }
      }
    }

    if (this->schemas.contains(string_identifier)) {
      return this->schemas.at(string_identifier);
    }

    // Fallback resolution logic
    const sourcemeta::core::URI uri{std::string{identifier}};
    if (uri.is_file()) {
      const auto path{uri.to_path()};
      log_verbose(this->options_)
          << "Attempting to read file reference from disk: " << path.string()
          << "\n";
      if (std::filesystem::exists(path)) {
        return std::optional<sourcemeta::core::JSON>{
            sourcemeta::core::read_yaml_or_json(path)};
      }
    }

    if (this->remote_) {
      return fallback_resolver(this->options_, identifier);
    } else {
      return sourcemeta::core::schema_official_resolver(identifier);
    }
  }

private:
  std::map<std::string, sourcemeta::core::JSON> schemas{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::core::SchemaConfig> configuration_;
  bool remote_{false};
};

inline auto
resolver(const sourcemeta::core::Options &options, const bool remote,
         const std::optional<std::string> &default_dialect,
         const std::optional<sourcemeta::core::SchemaConfig> &configuration)
    -> const CustomResolver & {
  using CacheKey = std::pair<bool, std::optional<std::string>>;
  static std::map<CacheKey, CustomResolver> resolver_cache;
  const CacheKey cache_key{remote, default_dialect};

  // Check if resolver is already cached
  auto iterator{resolver_cache.find(cache_key)};
  if (iterator != resolver_cache.end()) {
    return iterator->second;
  }

  // Construct resolver directly in cache
  auto [inserted_iterator, inserted] = resolver_cache.emplace(
      std::piecewise_construct, std::forward_as_tuple(cache_key),
      std::forward_as_tuple(options, configuration, remote, default_dialect));
  return inserted_iterator->second;
}

} // namespace sourcemeta::jsonschema::cli

#endif
