#include <sourcemeta/hydra/httpclient.h>
#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "utils.h"

#include <cassert>   // assert
#include <fstream>   // std::ofstream
#include <iostream>  // std::cerr
#include <optional>  // std::optional, std::nullopt
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

namespace {

auto handle_json_entry(
    const std::filesystem::path &entry_path,
    std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
        &result) -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      if (!std::filesystem::is_directory(entry) &&
          entry.path().extension() == ".json") {
        result.emplace_back(entry.path(),
                            sourcemeta::jsontoolkit::from_file(entry.path()));
      }
    }
  } else {
    if (!std::filesystem::exists(entry_path)) {
      std::ostringstream error;
      error << "No such file or directory: " << entry_path.string();
      throw std::runtime_error(error.str());
    }

    result.emplace_back(entry_path,
                        sourcemeta::jsontoolkit::from_file(entry_path));
  }
}

} // namespace

namespace intelligence::jsonschema::cli {

auto for_each_json(const std::vector<std::string> &arguments)
    -> std::vector<
        std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>> {
  std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
      result;

  if (arguments.empty()) {
    handle_json_entry(std::filesystem::current_path(), result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, result);
    }
  }

  return result;
}

auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>> {
  std::map<std::string, std::vector<std::string>> options;
  std::set<std::string> effective_flags{flags};
  effective_flags.insert("v");
  effective_flags.insert("verbose");

  options.insert({"", {}});
  std::optional<std::string> current_option;
  for (const auto &argument : arguments) {
    // Long option
    if (argument.starts_with("--") && argument.size() > 2) {
      current_option = argument.substr(2);
      assert(current_option.has_value());
      assert(!current_option.value().empty());
      options.insert({current_option.value(), {}});
      assert(options.contains(current_option.value()));
      if (effective_flags.contains(current_option.value())) {
        current_option = std::nullopt;
      }

      // Short option
    } else if (argument.starts_with("-") && argument.size() == 2) {
      current_option = argument.substr(1);
      assert(current_option.has_value());
      assert(current_option.value().size() == 1);
      options.insert({current_option.value(), {}});
      assert(options.contains(current_option.value()));
      if (effective_flags.contains(current_option.value())) {
        current_option = std::nullopt;
      }

      // Option value
    } else if (current_option.has_value()) {
      assert(options.contains(current_option.value()));
      options.at(current_option.value()).emplace_back(argument);
      current_option = std::nullopt;
      // Positional
    } else {
      assert(options.contains(""));
      options.at("").emplace_back(argument);
    }
  }

  return options;
}

auto pretty_evaluate_callback(
    bool result,
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
    const sourcemeta::jsontoolkit::Pointer &evaluate_path,
    const sourcemeta::jsontoolkit::Pointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &,
    const sourcemeta::jsontoolkit::JSON &) -> void {
  if (result) {
    return;
  }

  std::cerr << "error: " << sourcemeta::jsontoolkit::describe(step) << "\n";
  std::cerr << "    at instance location \"";
  sourcemeta::jsontoolkit::stringify(instance_location, std::cerr);
  std::cerr << "\"\n";

  std::cerr << "    at evaluate path \"";
  sourcemeta::jsontoolkit::stringify(evaluate_path, std::cerr);
  std::cerr << "\"\n";
}

static auto fallback_resolver(
    const std::map<std::string, std::vector<std::string>> &options,
    std::string_view identifier)
    -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
  auto official_result{
      sourcemeta::jsontoolkit::official_resolver(identifier).get()};
  if (official_result.has_value()) {
    std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
    promise.set_value(std::move(official_result));
    return promise.get_future();
  }

  // If the URI is not an HTTP URL, then abort
  const sourcemeta::jsontoolkit::URI uri{std::string{identifier}};
  const auto maybe_scheme{uri.scheme()};
  if (uri.is_urn() || !maybe_scheme.has_value() ||
      (maybe_scheme.value() != "https" && maybe_scheme.value() != "http")) {
    std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
    promise.set_value(std::nullopt);
    return promise.get_future();
  }

  log_verbose(options) << "Attempting to fetch over HTTP: " << identifier
                       << "\n";
  sourcemeta::hydra::http::ClientRequest request{std::string{identifier}};
  request.method(sourcemeta::hydra::http::Method::GET);
  sourcemeta::hydra::http::ClientResponse response{request.send().get()};
  if (response.status() != sourcemeta::hydra::http::Status::OK) {
    std::ostringstream error;
    error << "Failed to fetch " << identifier
          << " over HTTP. Got status code: " << response.status();
    throw std::runtime_error(error.str());
  }

  std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
  promise.set_value(sourcemeta::jsontoolkit::parse(response.body()));
  return promise.get_future();
}

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote) -> sourcemeta::jsontoolkit::SchemaResolver {
  sourcemeta::jsontoolkit::MapSchemaResolver dynamic_resolver{
      [&remote, &options](std::string_view identifier) {
        if (remote) {
          return fallback_resolver(options, identifier);
        } else {
          return sourcemeta::jsontoolkit::official_resolver(identifier);
        }
      }};

  if (options.contains("resolve")) {
    for (const auto &schema_path : options.at("resolve")) {
      log_verbose(options) << "Loading schema: " << schema_path << "\n";
      dynamic_resolver.add(sourcemeta::jsontoolkit::from_file(schema_path));
    }
  }

  if (options.contains("r")) {
    for (const auto &schema_path : options.at("r")) {
      log_verbose(options) << "Loading schema: " << schema_path << "\n";
      dynamic_resolver.add(sourcemeta::jsontoolkit::from_file(schema_path));
    }
  }

  return dynamic_resolver;
}

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream & {
  if (options.contains("verbose") || options.contains("v")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

auto validate_against_metaschema(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver) -> bool {
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema)};
  if (!dialect.has_value()) {
    throw std::runtime_error(
        "Cannot determine the dialect of the input schema");
  }

  const auto metaschema{resolver(dialect.value()).get()};
  if (!metaschema.has_value()) {
    throw sourcemeta::jsontoolkit::SchemaResolutionError(
        dialect.value(), "Could not resolve metaschema");
  }

  const auto metaschema_template{sourcemeta::jsontoolkit::compile(
      metaschema.value(), sourcemeta::jsontoolkit::default_schema_walker,
      resolver, sourcemeta::jsontoolkit::default_schema_compiler)};
  return sourcemeta::jsontoolkit::evaluate(
      metaschema_template, schema,
      sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
      pretty_evaluate_callback);
}

} // namespace intelligence::jsonschema::cli
