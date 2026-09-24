#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>

#include <sourcemeta/blaze/compiler.h>

#include <cstdint>     // std::uint8_t
#include <cstdlib>     // EXIT_SUCCESS
#include <filesystem>  // std::filesystem::current_path
#include <iostream>    // std::cerr, std::cout
#include <numeric>     // std::accumulate
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <sstream>     // std::ostringstream
#include <string_view> // std::string_view
#include <utility>     // std::pair

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "print.h"
#include "resolver.h"
#include "utils.h"

namespace {

using sourcemeta::core::TerminalStyle;
constexpr auto FAILURE_STYLE{TerminalStyle::Bold | TerminalStyle::Red};
constexpr auto MESSAGE_STYLE{TerminalStyle::Bold};
constexpr auto IDENTIFIER_STYLE{TerminalStyle::Bold | TerminalStyle::Cyan};
constexpr auto LOCATION_STYLE{TerminalStyle::Cyan};

} // namespace

constexpr std::string_view EXCLUDE_KEYWORD{"x-lint-exclude"};

template <typename Options, typename Iterator>
static auto disable_lint_rules(sourcemeta::blaze::SchemaTransformer &bundle,
                               const Options &options, Iterator first,
                               Iterator last) -> void {
  for (auto iterator = first; iterator != last; ++iterator) {
    if (bundle.remove(*iterator)) {
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
        if (entry.from_stdin) {
          std::cout << entry.first;
        } else {
          std::cout << std::filesystem::relative(entry.resolution_base)
                           .generic_string();
        }
        if (position.has_value()) {
          const auto [line, column, end_line, end_column] = position.value();
          std::cout << ":";
          std::cout << line;
          std::cout << ":";
          std::cout << column;
        } else {
          std::cout << ":<unknown>:<unknown>";
        }

        std::cout << ":\n";
        std::cout << "  ";
        if (sourcemeta::core::terminal_color_enabled(
                sourcemeta::core::TerminalStream::Stdout)) {
          std::cout << sourcemeta::jsonschema::paint("✗", FAILURE_STYLE) << " ";
        }
        std::cout << sourcemeta::jsonschema::paint(message, MESSAGE_STYLE)
                  << " ("
                  << sourcemeta::jsonschema::paint(name, IDENTIFIER_STYLE)
                  << ")\n";
        std::ostringstream pointer_stream;
        sourcemeta::core::stringify(schema_location, pointer_stream);
        std::cout << "    "
                  << sourcemeta::jsonschema::paint("at location",
                                                   TerminalStyle::Bold)
                  << " \""
                  << sourcemeta::jsonschema::paint(pointer_stream.str(),
                                                   LOCATION_STYLE)
                  << "\"\n";

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

// An OpenAPI description declares no identifier of its own under the revisions
// we support, so the one it is linted under is where it came from
static auto openapi_default_id(const sourcemeta::jsonschema::InputJSON &entry)
    -> std::string {
  if (entry.from_stdin) {
    return std::string{sourcemeta::jsonschema::STDIN_OPENAPI_DEFAULT_ID};
  }

  return sourcemeta::jsonschema::default_id(entry);
}

// An OpenAPI description that comes from standard input is not a schema, so it
// goes by an identifier of its own wherever we report on it
static auto
retag_openapi_stdin(std::vector<sourcemeta::jsonschema::InputJSON> &entries)
    -> void {
  for (auto &entry : entries) {
    if (entry.from_stdin &&
        sourcemeta::core::openapi_version(entry.second).has_value()) {
      entry.first =
          std::string{sourcemeta::jsonschema::STDIN_OPENAPI_DEFAULT_ID};
      entry.resolution_base = sourcemeta::jsonschema::openapi_stdin_path();
    }
  }
}

// A description of a revision we cannot read is no JSON Schema either, so it is
// turned down rather than linted as one
static auto
reject_unsupported_openapi(const sourcemeta::jsonschema::InputJSON &entry)
    -> void {
  const auto *version{
      sourcemeta::jsonschema::unsupported_openapi_version(entry.second)};
  if (version != nullptr) {
    throw sourcemeta::core::FileError<
        sourcemeta::jsonschema::UnsupportedOpenAPIVersionError>(
        entry.resolution_base, version->to_string());
  }
}

static auto
check_openapi(const sourcemeta::blaze::SchemaTransformer &bundle,
              const sourcemeta::jsonschema::InputJSON &entry,
              const sourcemeta::core::SchemaResolver &resolver,
              const sourcemeta::blaze::SchemaTransformer::Callback &callback)
    -> std::pair<bool, std::uint8_t> {
  const sourcemeta::core::OpenAPIFrame frame{
      entry.second, sourcemeta::core::schema_walker, resolver,
      openapi_default_id(entry)};
  return bundle.check(entry.second, frame.schemas(),
                      sourcemeta::core::schema_walker, resolver, callback,
                      sourcemeta::core::JSON::String{EXCLUDE_KEYWORD});
}

static auto
apply_openapi(const sourcemeta::blaze::SchemaTransformer &bundle,
              sourcemeta::core::JSON &document,
              const sourcemeta::jsonschema::InputJSON &entry,
              const sourcemeta::core::SchemaResolver &resolver,
              const sourcemeta::blaze::SchemaTransformer::Callback &callback)
    -> std::pair<bool, std::uint8_t> {
  const auto default_base{openapi_default_id(entry)};
  std::optional<sourcemeta::core::OpenAPIFrame> frame;
  return bundle.apply(
      document,
      [&frame, &resolver, &default_base](const sourcemeta::core::JSON &current)
          -> const sourcemeta::core::SchemaFrame & {
        frame.emplace(current, sourcemeta::core::schema_walker, resolver,
                      default_base);
        return frame.value().schemas();
      },
      sourcemeta::core::schema_walker, resolver, callback,
      sourcemeta::core::JSON::String{EXCLUDE_KEYWORD});
}

static auto load_rule(sourcemeta::blaze::SchemaTransformer &bundle,
                      std::unordered_set<std::string> &rule_names,
                      const std::filesystem::path &rule_path,
                      const std::string_view dialect,
                      const sourcemeta::core::SchemaResolver &custom_resolver,
                      const std::optional<sourcemeta::blaze::Tweaks> &tweaks,
                      const sourcemeta::blaze::SchemaRule::Scope scope)
    -> void {
  auto rule_schema{sourcemeta::core::read_yaml_or_json(rule_path)};
  if (!rule_schema.defines("description")) {
    rule_schema.assign("description",
                       sourcemeta::core::JSON{"<no description>"});
  }

  if (rule_schema.defines("title") && rule_schema.at("title").is_string()) {
    const auto rule_name{rule_schema.at("title").to_string()};
    if (rule_names.contains(rule_name)) {
      throw sourcemeta::core::FileError<
          sourcemeta::jsonschema::DuplicateLintRuleError>(rule_path, rule_name);
    }

    rule_names.emplace(rule_name);
  }

  try {
    bundle.add<sourcemeta::blaze::SchemaRule>(
        rule_schema, sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::blaze::default_schema_compiler, dialect, tweaks, scope);
  } catch (const sourcemeta::blaze::SchemaRuleMissingNameError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRuleMissingNameError>(rule_path, error);
  } catch (const sourcemeta::blaze::SchemaRuleInvalidNameError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRuleInvalidNameError>(rule_path, error);
  } catch (const sourcemeta::blaze::SchemaRuleInvalidNamePatternError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRuleInvalidNamePatternError>(rule_path, error);
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(rule_path,
                                                                  error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(rule_path);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(rule_path);
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaVocabularyError>(
        rule_path, error.uri(), error.what());
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        rule_path, error);
  }
}

static auto
load_rules_from_options(sourcemeta::blaze::SchemaTransformer &bundle,
                        std::unordered_set<std::string> &rule_names,
                        const sourcemeta::core::Options &options,
                        const std::string_view option_name,
                        const sourcemeta::blaze::SchemaRule::Scope scope)
    -> void {
  for (const auto &rule_path_string : options.at(option_name)) {
    const std::filesystem::path rule_path{
        std::filesystem::weakly_canonical(rule_path_string)};
    sourcemeta::jsonschema::LOG_VERBOSE(options)
        << "Loading custom rule: " << rule_path.generic_string() << "\n";
    const auto configuration_path{
        sourcemeta::jsonschema::find_configuration(options, rule_path)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path, rule_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &custom_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};
    load_rule(bundle, rule_names, rule_path, dialect, custom_resolver,
              sourcemeta::jsonschema::format_assertion_tweaks(options), scope);
  }
}

auto sourcemeta::jsonschema::lint(const sourcemeta::core::Options &options)
    -> void {
  validate_http_headers(options);
  const bool output_json = options.contains("json");

  sourcemeta::blaze::SchemaTransformer bundle;
  sourcemeta::blaze::add(bundle, sourcemeta::blaze::AlterSchemaMode::Linter);

  std::unordered_set<std::string> rule_names;
  for (const auto &entry : bundle) {
    const auto &[rule, check_flag, fix_flag] = entry;
    rule_names.emplace(rule->name());
  }

  std::unordered_set<std::string> seen_configurations;
  std::unordered_set<sourcemeta::core::JSON::String> configuration_excludes;
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
    const auto configuration_path{find_configuration(options, input_path)};
    if (!configuration_path.has_value()) {
      continue;
    }

    const auto canonical_configuration_path{
        std::filesystem::weakly_canonical(configuration_path.value())
            .generic_string()};
    if (seen_configurations.contains(canonical_configuration_path)) {
      continue;
    }

    seen_configurations.emplace(canonical_configuration_path);
    const auto &configuration{read_configuration(options, configuration_path)};
    if (!configuration.has_value()) {
      continue;
    }

    configuration_excludes.insert(configuration.value().lint.exclude.cbegin(),
                                  configuration.value().lint.exclude.cend());

    if (configuration.value().lint.rules.empty()) {
      continue;
    }

    const auto dialect{default_dialect(options, configuration)};
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};
    for (const auto &rule : configuration.value().lint.rules) {
      LOG_VERBOSE(options) << "Loading custom rule from configuration: "
                           << rule.path.generic_string() << "\n";
      load_rule(bundle, rule_names, rule.path, dialect, custom_resolver,
                sourcemeta::jsonschema::format_assertion_tweaks(options),
                rule.top_level ? sourcemeta::blaze::SchemaRule::Scope::TopLevel
                               : sourcemeta::blaze::SchemaRule::Scope::All);
    }
  }

  if (options.contains("rule")) {
    load_rules_from_options(bundle, rule_names, options, "rule",
                            sourcemeta::blaze::SchemaRule::Scope::All);
  }

  if (options.contains("top-level-rule")) {
    load_rules_from_options(bundle, rule_names, options, "top-level-rule",
                            sourcemeta::blaze::SchemaRule::Scope::TopLevel);
  }

  if (options.contains("only")) {
    if (options.contains("exclude")) {
      throw OptionConflictError{
          "Cannot use --only and --exclude at the same time"};
    }

    std::unordered_set<std::string_view> blacklist;
    for (const auto &[rule, check_flag, fix_flag] : bundle) {
      blacklist.emplace(rule->name());
    }

    for (const auto &only : options.at("only")) {
      LOG_VERBOSE(options) << "Only enabling rule: " << only << "\n";
      if (blacklist.erase(only) == 0) {
        throw InvalidLintRuleError{"The following linting rule does not exist",
                                   std::string{only}};
      }
    }

    for (const auto &name : blacklist) {
      bundle.remove(name);
    }
  } else if (options.contains("exclude")) {
    disable_lint_rules(bundle, options, options.at("exclude").cbegin(),
                       options.at("exclude").cend());
  }

  if (!options.contains("only") && !configuration_excludes.empty()) {
    std::vector<sourcemeta::core::JSON::String> sorted_excludes{
        configuration_excludes.cbegin(), configuration_excludes.cend()};
    std::sort(sorted_excludes.begin(), sorted_excludes.end());
    for (const auto &exclude : sorted_excludes) {
      if (bundle.remove(exclude)) {
        LOG_VERBOSE(options)
            << "Disabling rule from configuration: " << exclude << "\n";
      }
    }
  }

  if (options.contains("list")) {
    std::vector<std::pair<std::string_view, std::string_view>> rules;
    for (const auto &[rule, check_flag, fix_flag] : bundle) {
      rules.emplace_back(rule->name(), rule->message());
    }

    std::sort(
        rules.begin(), rules.end(), [](const auto &left, const auto &right) {
          return left.first < right.first ||
                 (left.first == right.first && left.second < right.second);
        });

    std::size_t count{0};
    for (const auto &entry : rules) {
      std::cout << sourcemeta::jsonschema::paint(entry.first, IDENTIFIER_STYLE)
                << "\n";
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
    auto entries = for_each_json(options, InputFormatting::Preserve);
    retag_openapi_stdin(entries);

    for (const auto &entry : entries) {
      const auto configuration_path{
          find_configuration(options, entry.resolution_base)};
      const auto &configuration{read_configuration(options, configuration_path,
                                                   entry.resolution_base)};
      const auto dialect{default_dialect(options, configuration)};

      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      LOG_VERBOSE(options) << "Linting: " << entry.first << "\n";
      if (entry.multidocument) {
        throw MultiDocumentInputError{
            "The --fix option is not supported for input with multiple "
            "documents",
            entry.resolution_base};
      }

      if (!entry.second.is_object() && !entry.second.is_boolean()) {
        throw NotSchemaError{entry.resolution_base};
      }

      reject_unsupported_openapi(entry);

      const auto is_openapi{
          sourcemeta::core::openapi_version(entry.second).has_value()};
      if (is_openapi && format_output) {
        throw sourcemeta::core::FileError<UnsupportedOpenAPIFormatError>(
            entry.resolution_base);
      }

      auto copy = entry.second;
      bool printed_progress{false};

      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto apply_result =
                  is_openapi
                      ? apply_openapi(bundle, copy, entry, custom_resolver,
                                      get_lint_callback(errors_array, entry,
                                                        output_json, true,
                                                        printed_progress))
                      : bundle.apply(
                            copy, sourcemeta::core::schema_walker,
                            custom_resolver,
                            get_lint_callback(errors_array, entry, output_json,
                                              true, printed_progress),
                            dialect, sourcemeta::jsonschema::default_id(entry),
                            sourcemeta::core::JSON::String{EXCLUDE_KEYWORD});
              if (printed_progress) {
                std::cerr << "\n";
              }
              scores.emplace_back(apply_result.second);
              if (!apply_result.first) {
                return EXIT_EXPECTED_FAILURE;
              }

              return EXIT_SUCCESS;
            } catch (const sourcemeta::core::OpenAPIError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::core::OpenAPIError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>(
                  entry.resolution_base, error);
            } catch (
                const sourcemeta::blaze::SchemaTransformRuleProcessedTwiceError
                    &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw LintAutoFixError{error.what(), entry.resolution_base,
                                     error.location()};
            } catch (
                const sourcemeta::blaze::SchemaBrokenReferenceError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw LintAutoFixError{
                  "Could not autofix the schema without breaking its internal "
                  "references",
                  entry.resolution_base, error.location()};
            } catch (
                const sourcemeta::blaze::CompilerInvalidRegexError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerInvalidRegexError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::blaze::CompilerError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::blaze::CompilerError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerError>(entry.resolution_base,
                                                    error);
            } catch (
                const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError
                    &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaKeywordError>(entry.resolution_base,
                                                        error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaFrameError>(entry.resolution_base,
                                                      error);
            } catch (
                const sourcemeta::core::SchemaAnchorCollisionError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::core::SchemaAnchorCollisionError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaAnchorCollisionError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaUnknownDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaVocabularyError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaVocabularyError>(
                  entry.resolution_base, error.uri(), error.what());
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaResolutionError>(
                  entry.resolution_base, error);
            } catch (...) {
              if (printed_progress) {
                std::cerr << "\n";
              }

              throw;
            }
          });

      if (wrapper_result == EXIT_SUCCESS ||
          wrapper_result == EXIT_EXPECTED_FAILURE) {
        if (wrapper_result != EXIT_SUCCESS) {
          result = false;
        }

        if (entry.from_stdin) {
          if (format_output) {
            if (!keep_ordering) {
              sourcemeta::jsonschema::format_schema(copy, custom_resolver,
                                                    dialect);
            }
          }

          sourcemeta::jsonschema::write_schema(copy, std::cout, indentation,
                                               entry.roundtrip);
        } else if (format_output) {
          if (!keep_ordering) {
            sourcemeta::jsonschema::format_schema(copy, custom_resolver,
                                                  dialect);
          }

          std::ostringstream expected;
          sourcemeta::jsonschema::write_schema(copy, expected, indentation,
                                               entry.roundtrip);

          const auto current{
              sourcemeta::core::read_file_to_string(entry.resolution_base)};

          if (current != expected.str()) {
            sourcemeta::core::atomic_write_file(entry.resolution_base,
                                                expected.str());
          }
        } else if (copy != entry.second) {
          sourcemeta::core::atomic_write_file(
              entry.resolution_base,
              [&copy, &indentation, &entry](std::ostream &stream) -> void {
                sourcemeta::jsonschema::write_schema(copy, stream, indentation,
                                                     entry.roundtrip);
              });
        }
      } else {
        throw Fail{wrapper_result};
      }
    }
  } else {
    auto entries = for_each_json(options);
    retag_openapi_stdin(entries);

    for (const auto &entry : entries) {
      const auto configuration_path{
          find_configuration(options, entry.resolution_base)};
      const auto &configuration{read_configuration(options, configuration_path,
                                                   entry.resolution_base)};
      const auto dialect{default_dialect(options, configuration)};
      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      if (!entry.second.is_object() && !entry.second.is_boolean()) {
        throw NotSchemaError{entry.resolution_base};
      }

      reject_unsupported_openapi(entry);

      LOG_VERBOSE(options) << "Linting: " << entry.first << "\n";

      const auto is_openapi{
          sourcemeta::core::openapi_version(entry.second).has_value()};
      bool printed_progress{false};
      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto subresult =
                  is_openapi
                      ? check_openapi(bundle, entry, custom_resolver,
                                      get_lint_callback(errors_array, entry,
                                                        output_json, false,
                                                        printed_progress))
                      : bundle.check(
                            entry.second, sourcemeta::core::schema_walker,
                            custom_resolver,
                            get_lint_callback(errors_array, entry, output_json,
                                              false, printed_progress),
                            dialect, sourcemeta::jsonschema::default_id(entry),
                            sourcemeta::core::JSON::String{EXCLUDE_KEYWORD});
              scores.emplace_back(subresult.second);
              if (subresult.first) {
                return EXIT_SUCCESS;
              }

              // Return 2 for logical lint failures
              return EXIT_EXPECTED_FAILURE;
            } catch (const sourcemeta::core::OpenAPIError &error) {
              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::core::OpenAPIError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>(
                  entry.resolution_base, error);
            } catch (
                const sourcemeta::blaze::CompilerInvalidRegexError &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerInvalidRegexError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::blaze::CompilerError &error) {
              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::blaze::CompilerError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerError>(entry.resolution_base,
                                                    error);
            } catch (
                const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError
                    &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaKeywordError>(entry.resolution_base,
                                                        error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaFrameError>(entry.resolution_base,
                                                      error);
            } catch (
                const sourcemeta::core::SchemaAnchorCollisionError &error) {
              const auto position{entry.positions.get(error.location())};
              if (position.has_value()) {
                throw PositionError<sourcemeta::core::FileError<
                    sourcemeta::core::SchemaAnchorCollisionError>>(
                    std::get<0>(position.value()),
                    std::get<1>(position.value()), entry.resolution_base,
                    error);
              }

              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaAnchorCollisionError>(
                  entry.resolution_base, error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaUnknownDialectError>(
                  entry.resolution_base);
            } catch (const sourcemeta::core::SchemaVocabularyError &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaVocabularyError>(
                  entry.resolution_base, error.uri(), error.what());
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              throw sourcemeta::core::FileError<
                  sourcemeta::core::SchemaResolutionError>(
                  entry.resolution_base, error);
            }
          });

      if (wrapper_result == EXIT_EXPECTED_FAILURE) {
        result = false;
      } else if (wrapper_result != EXIT_SUCCESS) {
        throw Fail{wrapper_result};
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
    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
