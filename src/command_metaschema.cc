#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <cassert> // assert
#include <filesystem>
#include <iostream> // std::cerr
#include <map>      // std::map
#include <optional>
#include <sstream>
#include <string> // std::string
#include <string_view>
#include <vector>

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::metaschema(
    const sourcemeta::core::Options &options) -> void {
  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  bool result{true};
  sourcemeta::blaze::Evaluator evaluator;

  std::map<std::string, sourcemeta::blaze::Template> cache;

  std::vector<std::string_view> local_arguments;
  std::vector<std::string_view> url_arguments;

  if (!options.positional().empty()) {
    for (const auto &argument : options.positional()) {
      if (sourcemeta::jsonschema::is_http_url(argument)) {
        url_arguments.push_back(argument);
      } else {
        local_arguments.push_back(argument);
      }
    }
  }

  const auto local_entries =
      options.positional().empty()
          ? for_each_json(options)
          : (local_arguments.empty() ? std::vector<InputJSON>{}
                                     : for_each_json(local_arguments, options));

  if (!url_arguments.empty() && !options.contains("http")) {
    throw std::runtime_error{
        "Remote schema inputs require network access. Pass `--http/-h`"};
  }

  const std::optional<sourcemeta::core::SchemaConfig> *remote_configuration{
      nullptr};
  std::string_view remote_default_dialect_option;
  const sourcemeta::jsonschema::CustomResolver *remote_resolver{nullptr};
  if (!url_arguments.empty()) {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    remote_configuration = &read_configuration(options, configuration_path);
    remote_default_dialect_option =
        default_dialect(options, *remote_configuration);
    remote_resolver =
        &resolver(options, /* remote */ true, remote_default_dialect_option,
                  *remote_configuration);
  }

  const auto process_schema =
      [&](const sourcemeta::core::JSON &schema,
          const sourcemeta::core::PointerPositionTracker &positions,
          const std::optional<std::filesystem::path> &schema_path,
          const std::string_view schema_display,
          const sourcemeta::jsonschema::CustomResolver &custom_resolver,
          const std::string_view default_dialect_option) -> void {
    if (!sourcemeta::core::is_schema(schema)) {
      if (schema_path.has_value()) {
        throw NotSchemaError{schema_path.value()};
      }

      throw RemoteSchemaNotSchemaError{std::string{schema_display}};
    }

    try {
      const auto dialect{
          sourcemeta::core::dialect(schema, default_dialect_option)};
      if (dialect.empty()) {
        if (schema_path.has_value()) {
          throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
              schema_path.value());
        }

        std::ostringstream error;
        error << "Could not resolve the metaschema of the schema\n  at uri "
              << schema_display;
        throw std::runtime_error(error.str());
      }

      const auto metaschema{sourcemeta::core::metaschema(
          schema, custom_resolver, default_dialect_option)};
      const sourcemeta::core::JSON bundled{
          sourcemeta::core::bundle(metaschema, sourcemeta::core::schema_walker,
                                   custom_resolver, default_dialect_option)};
      sourcemeta::core::SchemaFrame frame{
          sourcemeta::core::SchemaFrame::Mode::References};
      frame.analyse(bundled, sourcemeta::core::schema_walker, custom_resolver,
                    default_dialect_option);

      if (!cache.contains(std::string{dialect})) {
        const auto metaschema_template{sourcemeta::blaze::compile(
            bundled, sourcemeta::core::schema_walker, custom_resolver,
            sourcemeta::blaze::default_schema_compiler, frame,
            sourcemeta::blaze::Mode::Exhaustive, default_dialect_option)};
        cache.insert({std::string{dialect}, metaschema_template});
      }

      if (trace) {
        sourcemeta::blaze::TraceOutput output{
            sourcemeta::core::schema_walker, custom_resolver,
            sourcemeta::core::empty_weak_pointer, frame};
        result = evaluator.validate(cache.at(std::string{dialect}), schema,
                                    std::ref(output));
        print(output, positions, std::cout);
      } else if (json_output) {
        // Otherwise its impossible to correlate the output
        // when validating i.e. a directory of schemas
        std::cerr << schema_display << "\n";
        const auto output{sourcemeta::blaze::standard(
            evaluator, cache.at(std::string{dialect}), schema,
            sourcemeta::blaze::StandardOutput::Basic, positions)};
        assert(output.is_object());
        assert(output.defines("valid"));
        assert(output.at("valid").is_boolean());
        if (!output.at("valid").to_boolean()) {
          result = false;
        }

        sourcemeta::core::prettify(output, std::cout);
        std::cout << "\n";
      } else {
        sourcemeta::blaze::SimpleOutput output{schema};
        if (evaluator.validate(cache.at(std::string{dialect}), schema,
                               std::ref(output))) {
          LOG_VERBOSE(options)
              << "ok: " << schema_display << "\n  matches " << dialect << "\n";
        } else {
          std::cerr << "fail: " << schema_display << "\n";
          print(output, positions, std::cerr);
          result = false;
        }
      }
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      if (schema_path.has_value()) {
        throw FileError<
            sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    }
  };

  for (const auto &entry : local_entries) {
    const auto configuration_path{find_configuration(entry.first)};
    const auto &configuration{
        read_configuration(options, configuration_path, entry.first)};
    const auto default_dialect_option{default_dialect(options, configuration)};
    const auto &custom_resolver{resolver(options, options.contains("http"),
                                         default_dialect_option,
                                         configuration)};

    process_schema(entry.second, entry.positions, entry.first,
                   sourcemeta::core::weakly_canonical(entry.first).string(),
                   custom_resolver, default_dialect_option);
  }

  if (remote_resolver != nullptr && remote_configuration != nullptr) {
    for (const auto &uri : url_arguments) {
      const auto fetched{
          sourcemeta::jsonschema::fetch_http_schema(options, uri)};
      sourcemeta::core::PointerPositionTracker positions;
      process_schema(fetched, positions, std::nullopt, uri, *remote_resolver,
                     remote_default_dialect_option);
    }
  }

  if (!result) {
    // Report a different exit code for validation failures, to
    // distinguish them from other errors
    throw Fail{2};
  }
}
