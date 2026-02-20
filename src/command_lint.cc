#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/linter.h>

#include <cstdlib>    // EXIT_FAILURE
#include <filesystem> // std::filesystem::current_path
#include <fstream>    // std::ofstream, std::ifstream
#include <iostream>   // std::cerr, std::cout
#include <numeric>    // std::accumulate
#include <sstream>    // std::ostringstream

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

static const sourcemeta::core::JSON::String EXCLUDE_KEYWORD{"x-lint-exclude"};

template <typename Options, typename Iterator>
static auto disable_lint_rules(sourcemeta::core::SchemaTransformer &bundle,
                               const Options &options, Iterator first,
                               Iterator last) -> void {
  for (auto iterator = first; iterator != last; ++iterator) {
    if (bundle.remove(std::string{*iterator})) {
      sourcemeta::jsonschema::LOG_VERBOSE(options)
          << "Disabling rule: " << *iterator << "\n";
    } else {
      sourcemeta::jsonschema::LOG_WARNING()
          << "Cannot exclude unknown rule: " << *iterator << "\n";
    }
  }
}

static auto reindent(const std::string_view &value,
                     const std::string &indentation, std::ostream &stream)
    -> void {
  if (!value.empty()) {
    stream << indentation;
  }

  for (std::size_t index = 0; index < value.size(); index++) {
    const auto character{value[index]};
    stream.put(character);
    if (character == '\n' && index != value.size() - 1) {
      stream << indentation;
    }
  }
}

static auto get_lint_callback(sourcemeta::core::JSON &errors_array,
                              const sourcemeta::jsonschema::InputJSON &entry,
                              const bool output_json, const bool fixing,
                              bool &printed_progress) -> auto {
  return [&entry, &errors_array, output_json, fixing, &printed_progress](
             const auto &pointer, const auto &name, const auto &message,
             const auto &result, const auto applied) {
    if (fixing && applied) {
      if (!output_json) {
        std::cerr << ".";
        printed_progress = true;
      }

      return;
    }

    if (printed_progress) {
      std::cerr << "\n";
      printed_progress = false;
    }

    std::vector<sourcemeta::core::Pointer> locations;
    if (result.locations.empty()) {
      locations.emplace_back();
    } else {
      for (const auto &location : result.locations) {
        locations.push_back(location);
      }
    }

    for (const auto &location : locations) {
      const auto schema_location{pointer.concat(location)};
      const auto position{entry.positions.get(schema_location)};

      if (output_json) {
        auto error_obj = sourcemeta::core::JSON::make_object();

        error_obj.assign("path", sourcemeta::core::JSON{entry.first});
        error_obj.assign("id", sourcemeta::core::JSON{name});
        error_obj.assign("message", sourcemeta::core::JSON{message});
        error_obj.assign("description",
                         sourcemeta::core::to_json(result.description));
        error_obj.assign("schemaLocation",
                         sourcemeta::core::to_json(schema_location));
        if (position.has_value()) {
          error_obj.assign("position",
                           sourcemeta::core::to_json(position.value()));
        } else {
          error_obj.assign("position", sourcemeta::core::to_json(nullptr));
        }

        errors_array.push_back(error_obj);
      } else {
        std::cout << std::filesystem::relative(entry.resolution_base).string();
        if (position.has_value()) {
          std::cout << ":";
          std::cout << std::get<0>(position.value());
          std::cout << ":";
          std::cout << std::get<1>(position.value());
        } else {
          std::cout << ":<unknown>:<unknown>";
        }

        std::cout << ":\n";
        std::cout << "  " << message << " (" << name << ")\n";
        std::cout << "    at location \"";
        sourcemeta::core::stringify(schema_location, std::cout);
        std::cout << "\"\n";

        if (result.description.has_value()) {
          reindent(result.description.value(), "    ", std::cout);
          if (result.description.value().back() != '\n') {
            std::cout << "\n";
          }
        }
      }
    }
  };
}

static auto load_rule(sourcemeta::core::SchemaTransformer &bundle,
                      std::unordered_set<std::string> &rule_names,
                      const std::filesystem::path &rule_path,
                      const std::string_view dialect,
                      const sourcemeta::core::SchemaResolver &custom_resolver)
    -> void {
  auto rule_schema{sourcemeta::core::read_yaml_or_json(rule_path)};
  if (!rule_schema.defines("description")) {
    rule_schema.assign("description",
                       sourcemeta::core::JSON{"<no description>"});
  }

  if (rule_schema.defines("title") && rule_schema.at("title").is_string()) {
    const auto rule_name{rule_schema.at("title").to_string()};
    if (rule_names.contains(rule_name)) {
      throw sourcemeta::jsonschema::FileError<
          sourcemeta::jsonschema::DuplicateLintRuleError>(rule_path, rule_name);
    }

    rule_names.emplace(rule_name);
  }

  try {
    bundle.add<sourcemeta::blaze::SchemaRule>(
        rule_schema, sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::blaze::default_schema_compiler, dialect);
  } catch (const sourcemeta::blaze::LinterMissingNameError &error) {
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::blaze::LinterMissingNameError>(rule_path, error);
  } catch (const sourcemeta::blaze::LinterInvalidNameError &error) {
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::blaze::LinterInvalidNameError>(rule_path, error);
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(rule_path,
                                                                  error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(rule_path);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::core::SchemaResolutionError>(rule_path, error);
  }
}

auto sourcemeta::jsonschema::lint(const sourcemeta::core::Options &options)
    -> void {
  const bool output_json = options.contains("json");

  sourcemeta::core::SchemaTransformer bundle;
  sourcemeta::core::add(bundle, sourcemeta::core::AlterSchemaMode::Linter);

  bundle.add<sourcemeta::blaze::ValidExamples>(
      sourcemeta::blaze::default_schema_compiler);
  bundle.add<sourcemeta::blaze::ValidDefault>(
      sourcemeta::blaze::default_schema_compiler);

  std::unordered_set<std::string> rule_names;
  for (const auto &entry : bundle) {
    rule_names.emplace(std::get<0>(entry)->name());
  }

  std::unordered_set<std::string> seen_configurations;
  std::vector<std::filesystem::path> input_paths;
  if (options.positional().empty()) {
    input_paths.emplace_back(std::filesystem::current_path());
  } else {
    for (const auto &argument : options.positional()) {
      if (argument == "-") {
        input_paths.emplace_back(std::filesystem::current_path());
      } else {
        input_paths.emplace_back(std::filesystem::weakly_canonical(argument));
      }
    }
  }

  for (const auto &input_path : input_paths) {
    const auto configuration_path{find_configuration(input_path)};
    if (!configuration_path.has_value()) {
      continue;
    }

    const auto canonical_configuration_path{
        std::filesystem::weakly_canonical(configuration_path.value()).string()};
    if (seen_configurations.contains(canonical_configuration_path)) {
      continue;
    }

    seen_configurations.emplace(canonical_configuration_path);
    const auto &configuration{read_configuration(options, configuration_path)};
    if (!configuration.has_value() ||
        configuration.value().lint.rules.empty()) {
      continue;
    }

    const auto dialect{default_dialect(options, configuration)};
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};
    for (const auto &rule_path : configuration.value().lint.rules) {
      LOG_VERBOSE(options) << "Loading custom rule from configuration: "
                           << rule_path.string() << "\n";
      load_rule(bundle, rule_names, rule_path, dialect, custom_resolver);
    }
  }

  if (options.contains("rule")) {
    for (const auto &rule_path_string : options.at("rule")) {
      const std::filesystem::path rule_path{
          std::filesystem::weakly_canonical(rule_path_string)};
      LOG_VERBOSE(options) << "Loading custom rule: " << rule_path.string()
                           << "\n";
      const auto configuration_path{find_configuration(rule_path)};
      const auto &configuration{
          read_configuration(options, configuration_path, rule_path)};
      const auto dialect{default_dialect(options, configuration)};
      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      load_rule(bundle, rule_names, rule_path, dialect, custom_resolver);
    }
  }

  if (options.contains("only")) {
    if (options.contains("exclude")) {
      throw OptionConflictError{
          "Cannot use --only and --exclude at the same time"};
    }

    std::unordered_set<std::string_view> blacklist;
    for (const auto &entry : bundle) {
      blacklist.emplace(std::get<0>(entry)->name());
    }

    for (const auto &only : options.at("only")) {
      LOG_VERBOSE(options) << "Only enabling rule: " << only << "\n";
      if (blacklist.erase(only) == 0) {
        throw InvalidLintRuleError{"The following linting rule does not exist",
                                   std::string{only}};
      }
    }

    for (const auto &name : blacklist) {
      bundle.remove(std::string{name});
    }
  } else if (options.contains("exclude")) {
    disable_lint_rules(bundle, options, options.at("exclude").cbegin(),
                       options.at("exclude").cend());
  }

  if (options.contains("list")) {
    std::vector<std::pair<std::string_view, std::string_view>> rules;
    for (const auto &entry : bundle) {
      rules.emplace_back(std::get<0>(entry)->name(),
                         std::get<0>(entry)->message());
    }

    std::sort(
        rules.begin(), rules.end(), [](const auto &left, const auto &right) {
          return left.first < right.first ||
                 (left.first == right.first && left.second < right.second);
        });

    std::size_t count{0};
    for (const auto &entry : rules) {
      std::cout << entry.first << "\n";
      std::cout << "  " << entry.second << "\n\n";
      count += 1;
    }

    std::cout << "Number of rules: " << count << "\n";
    return;
  }

  const bool format_output{options.contains("format")};
  const bool keep_ordering{options.contains("keep-ordering")};

  if (format_output && !options.contains("fix")) {
    throw OptionConflictError{"The --format option requires --fix to be set"};
  }

  if (keep_ordering && !format_output) {
    throw OptionConflictError{
        "The --keep-ordering option requires --format to be set"};
  }

  bool result{true};
  auto errors_array = sourcemeta::core::JSON::make_array();
  std::vector<std::uint8_t> scores;
  const auto indentation{parse_indentation(options)};

  if (options.contains("fix")) {
    const auto entries = for_each_json(options);
    for (const auto &entry : entries) {
      if (entry.from_stdin) {
        throw StdinError{"The --fix option does not support standard input"};
      }
    }

    for (const auto &entry : entries) {
      const auto configuration_path{find_configuration(entry.resolution_base)};
      const auto &configuration{read_configuration(options, configuration_path,
                                                   entry.resolution_base)};
      const auto dialect{default_dialect(options, configuration)};

      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      LOG_VERBOSE(options) << "Linting: " << entry.first << "\n";
      if (entry.yaml) {
        throw YAMLInputError{
            "The --fix option is not supported for YAML input files",
            entry.resolution_base};
      }

      auto copy = entry.second;
      bool printed_progress{false};

      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto apply_result = bundle.apply(
                  copy, sourcemeta::core::schema_walker, custom_resolver,
                  get_lint_callback(errors_array, entry, output_json, true,
                                    printed_progress),
                  dialect, sourcemeta::jsonschema::default_id(entry),
                  EXCLUDE_KEYWORD);
              if (printed_progress) {
                std::cerr << "\n";
              }
              scores.emplace_back(apply_result.second);
              if (!apply_result.first) {
                return 2;
              }

              return EXIT_SUCCESS;
            } catch (
                const sourcemeta::core::SchemaTransformRuleProcessedTwiceError
                    &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw LintAutoFixError{error.what(), entry.resolution_base,
                                     error.location()};
            } catch (
                const sourcemeta::core::SchemaBrokenReferenceError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw LintAutoFixError{
                  "Could not autofix the schema without breaking its internal "
                  "references",
                  entry.resolution_base, error.location()};
            } catch (
                const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError
                    &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw FileError<
                  sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw FileError<sourcemeta::core::SchemaKeywordError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw FileError<sourcemeta::core::SchemaFrameError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw FileError<sourcemeta::core::SchemaResolutionError>(
                  entry.resolution_base, error);
            } catch (...) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw;
            }
          });

      if (wrapper_result == EXIT_SUCCESS || wrapper_result == 2) {
        if (wrapper_result != EXIT_SUCCESS) {
          result = false;
        }

        if (format_output) {
          if (!keep_ordering) {
            sourcemeta::core::format(copy, sourcemeta::core::schema_walker,
                                     custom_resolver, dialect);
          }

          std::ostringstream expected;
          sourcemeta::core::prettify(copy, expected, indentation);
          expected << "\n";

          std::ifstream current_stream{entry.resolution_base};
          std::ostringstream current;
          current << current_stream.rdbuf();

          if (current.str() != expected.str()) {
            std::ofstream output{entry.resolution_base};
            output << expected.str();
          }
        } else if (copy != entry.second) {
          std::ofstream output{entry.resolution_base};
          sourcemeta::core::prettify(copy, output, indentation);
          output << "\n";
        }
      } else {
        // Exception was caught - exit immediately with error code 1
        throw Fail{EXIT_FAILURE};
      }
    }
  } else {
    for (const auto &entry : for_each_json(options)) {
      const auto configuration_path{find_configuration(entry.resolution_base)};
      const auto &configuration{read_configuration(options, configuration_path,
                                                   entry.resolution_base)};
      const auto dialect{default_dialect(options, configuration)};
      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      LOG_VERBOSE(options) << "Linting: " << entry.first << "\n";

      bool printed_progress{false};
      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto subresult = bundle.check(
                  entry.second, sourcemeta::core::schema_walker,
                  custom_resolver,
                  get_lint_callback(errors_array, entry, output_json, false,
                                    printed_progress),
                  dialect, sourcemeta::jsonschema::default_id(entry),
                  EXCLUDE_KEYWORD);
              scores.emplace_back(subresult.second);
              if (subresult.first) {
                return EXIT_SUCCESS;
              } else {
                // Return 2 for logical lint failures
                return 2;
              }
            } catch (
                const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError
                    &error) {
              throw FileError<
                  sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              throw FileError<sourcemeta::core::SchemaKeywordError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              throw FileError<sourcemeta::core::SchemaFrameError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              throw FileError<sourcemeta::core::SchemaResolutionError>(
                  entry.resolution_base, error);
            }
          });

      if (wrapper_result == 2) {
        // Logical lint failure
        result = false;
      } else if (wrapper_result != EXIT_SUCCESS) {
        // Exception was caught - exit immediately with error code 1
        throw Fail{EXIT_FAILURE};
      }
    }
  }

  if (output_json) {
    std::sort(errors_array.as_array().begin(), errors_array.as_array().end(),
              [](const sourcemeta::core::JSON &left,
                 const sourcemeta::core::JSON &right) {
                return left.at("position").front() <
                       right.at("position").front();
              });

    auto output_json_object = sourcemeta::core::JSON::make_object();
    output_json_object.assign("valid", sourcemeta::core::JSON{result});

    if (scores.empty()) {
      output_json_object.assign("health", sourcemeta::core::JSON{nullptr});
    } else {
      const auto health{std::accumulate(scores.cbegin(), scores.cend(), 0ull) /
                        scores.size()};
      output_json_object.assign(
          "health", sourcemeta::core::JSON{static_cast<std::size_t>(health)});
    }

    output_json_object.assign("errors", sourcemeta::core::JSON{errors_array});
    sourcemeta::core::prettify(output_json_object, std::cout, indentation);
    std::cout << "\n";
  }

  if (!result) {
    // Report a different exit code for linting failures, to
    // distinguish them from other errors
    throw Fail{2};
  }
}
