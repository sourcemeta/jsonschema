#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/linter.h>

#include <cstdlib>  // EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout
#include <numeric>  // std::accumulate
#include <sstream>  // std::ostringstream

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
                              const bool output_json) -> auto {
  return [&entry, &errors_array,
          output_json](const auto &pointer, const auto &name,
                       const auto &message, const auto &result) {
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

        error_obj.assign("path", sourcemeta::core::JSON{entry.first.string()});
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
        std::cout << std::filesystem::relative(entry.first).string();
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

auto sourcemeta::jsonschema::lint(const sourcemeta::core::Options &options)
    -> void {
  const bool output_json = options.contains("json");

  sourcemeta::core::SchemaTransformer bundle;
  sourcemeta::core::add(bundle, sourcemeta::core::AlterSchemaMode::Linter);

  bundle.add<sourcemeta::blaze::ValidExamples>(
      sourcemeta::blaze::default_schema_compiler);
  bundle.add<sourcemeta::blaze::ValidDefault>(
      sourcemeta::blaze::default_schema_compiler);

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

  bool result{true};
  auto errors_array = sourcemeta::core::JSON::make_array();
  std::vector<std::uint8_t> scores;
  const auto indentation{parse_indentation(options)};

  if (options.contains("fix")) {
    for (const auto &entry : for_each_json(options)) {
      const auto configuration_path{find_configuration(entry.first)};
      const auto &configuration{
          read_configuration(options, configuration_path, entry.first)};
      const auto dialect{default_dialect(options, configuration)};

      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      LOG_VERBOSE(options) << "Linting: " << entry.first.string() << "\n";
      if (entry.yaml) {
        throw YAMLInputError{
            "The --fix option is not supported for YAML input files",
            entry.first};
      }

      auto copy = entry.second;

      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto apply_result = bundle.apply(
                  copy, sourcemeta::core::schema_walker, custom_resolver,
                  get_lint_callback(errors_array, entry, output_json), dialect,
                  sourcemeta::jsonschema::default_id(entry.first),
                  EXCLUDE_KEYWORD);
              scores.emplace_back(apply_result.second);
              if (!apply_result.first) {
                return 2;
              }

              return EXIT_SUCCESS;
            } catch (
                const sourcemeta::core::SchemaTransformRuleProcessedTwiceError
                    &error) {
              throw LintAutoFixError{error.what(), entry.first,
                                     error.location()};
            } catch (
                const sourcemeta::core::SchemaBrokenReferenceError &error) {
              throw LintAutoFixError{
                  "Could not autofix the schema without breaking its internal "
                  "references",
                  entry.first, error.location()};
            } catch (
                const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError
                    &error) {
              throw FileError<
                  sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
                  entry.first, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              throw FileError<sourcemeta::core::SchemaKeywordError>(entry.first,
                                                                    error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              throw FileError<sourcemeta::core::SchemaFrameError>(entry.first,
                                                                  error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.first);
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              throw FileError<sourcemeta::core::SchemaResolutionError>(
                  entry.first, error);
            }
          });

      if (wrapper_result == EXIT_SUCCESS || wrapper_result == 2) {
        if (wrapper_result != EXIT_SUCCESS) {
          result = false;
        }

        if (copy != entry.second) {
          std::ofstream output{entry.first};
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
      const auto configuration_path{find_configuration(entry.first)};
      const auto &configuration{
          read_configuration(options, configuration_path, entry.first)};
      const auto dialect{default_dialect(options, configuration)};
      const auto &custom_resolver{
          resolver(options, options.contains("http"), dialect, configuration)};
      LOG_VERBOSE(options) << "Linting: " << entry.first.string() << "\n";

      const auto wrapper_result =
          sourcemeta::jsonschema::try_catch(options, [&]() {
            try {
              const auto subresult = bundle.check(
                  entry.second, sourcemeta::core::schema_walker,
                  custom_resolver,
                  get_lint_callback(errors_array, entry, output_json), dialect,
                  sourcemeta::jsonschema::default_id(entry.first),
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
                  entry.first, error);
            } catch (const sourcemeta::core::SchemaKeywordError &error) {
              throw FileError<sourcemeta::core::SchemaKeywordError>(entry.first,
                                                                    error);
            } catch (const sourcemeta::core::SchemaFrameError &error) {
              throw FileError<sourcemeta::core::SchemaFrameError>(entry.first,
                                                                  error);
            } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
              throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
                  entry.first);
            } catch (const sourcemeta::core::SchemaResolutionError &error) {
              throw FileError<sourcemeta::core::SchemaResolutionError>(
                  entry.first, error);
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
