#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/linter.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout
#include <numeric>  // std::accumulate
#include <sstream>  // std::ostringstream

#include "command.h"
#include "error.h"
#include "utils.h"

template <typename Options, typename Iterator>
static auto disable_lint_rules(sourcemeta::core::SchemaTransformer &bundle,
                               const Options &options, Iterator first,
                               Iterator last) -> void {
  for (auto iterator = first; iterator != last; ++iterator) {
    if (bundle.remove(std::string{*iterator})) {
      sourcemeta::jsonschema::cli::log_verbose(options)
          << "Disabling rule: " << *iterator << "\n";
    } else {
      sourcemeta::jsonschema::cli::log_verbose(options)
          << "warning: Cannot exclude unknown rule: " << *iterator << "\n";
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

static auto
get_lint_callback(sourcemeta::core::JSON &errors_array,
                  const sourcemeta::jsonschema::cli::InputJSON &entry,
                  const bool output_json) -> auto {
  return [&entry, &errors_array,
          output_json](const auto &pointer, const auto &name,
                       const auto &message, const auto &result) {
    const auto schema_location{result.locations.empty()
                                   ? pointer
                                   : pointer.concat(result.locations.front())};
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
      std::cout << "    at schema location \"";
      sourcemeta::core::stringify(schema_location, std::cout);
      std::cout << "\"\n";

      if (result.description.has_value()) {
        reindent(result.description.value(), "    ", std::cout);
        if (result.description.value().back() != '\n') {
          std::cout << "\n";
        }
      }
    }
  };
}

auto sourcemeta::jsonschema::cli::lint(const sourcemeta::core::Options &options)
    -> int {
  const bool output_json = options.contains("json");

  sourcemeta::core::SchemaTransformer bundle;

  if (options.contains("strict")) {
    sourcemeta::core::add(bundle,
                          sourcemeta::core::AlterSchemaMode::ReadabilityStrict);
  } else {
    sourcemeta::core::add(bundle,
                          sourcemeta::core::AlterSchemaMode::Readability);
  }

  bundle.add<sourcemeta::blaze::ValidExamples>(
      sourcemeta::blaze::default_schema_compiler);
  bundle.add<sourcemeta::blaze::ValidDefault>(
      sourcemeta::blaze::default_schema_compiler);

  if (options.contains("only")) {
    if (options.contains("exclude")) {
      std::cerr << "error: Cannot use --only and --exclude at the same time\n";
      return EXIT_FAILURE;
    }

    std::unordered_set<std::string_view> blacklist;
    for (const auto &entry : bundle) {
      blacklist.emplace(entry.first);
    }

    for (const auto &only : options.at("only")) {
      log_verbose(options) << "Only enabling rule: " << only << "\n";
      if (blacklist.erase(only) == 0) {
        std::cerr << "error: The following linting rule does not exist\n";
        std::cerr << "  " << only << "\n";
        return EXIT_FAILURE;
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
    std::vector<std::pair<std::reference_wrapper<const std::string>,
                          std::reference_wrapper<const std::string>>>
        rules;
    for (const auto &entry : bundle) {
      rules.emplace_back(entry.first, entry.second->message());
    }

    std::sort(rules.begin(), rules.end(),
              [](const auto &left, const auto &right) {
                return left.first.get() < right.first.get() ||
                       (left.first.get() == right.first.get() &&
                        left.second.get() < right.second.get());
              });

    std::size_t count{0};
    for (const auto &entry : rules) {
      std::cout << entry.first.get() << "\n";
      std::cout << "  " << entry.second.get() << "\n\n";
      count += 1;
    }

    std::cout << "Number of rules: " << count << "\n";
    return EXIT_SUCCESS;
  }

  bool result{true};
  auto errors_array = sourcemeta::core::JSON::make_array();
  std::vector<std::uint8_t> scores;
  const auto dialect{default_dialect(options)};
  const auto custom_resolver{
      resolver(options, options.contains("http"), dialect)};

  if (options.contains("fix")) {
    for (const auto &entry :
         for_each_json(options.positional(), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      if (entry.first.extension() == ".yaml" ||
          entry.first.extension() == ".yml") {
        std::cerr << "The --fix option is not supported for YAML input files\n";
        return EXIT_FAILURE;
      }

      auto copy = entry.second;

      const auto wrapper_result = sourcemeta::jsonschema::try_catch([&]() {
        try {
          bundle.apply(
              copy, sourcemeta::core::schema_official_walker, custom_resolver,
              get_lint_callback(errors_array, entry, output_json), dialect,
              sourcemeta::core::URI::from_path(entry.first).recompose());
          return EXIT_SUCCESS;
        } catch (const sourcemeta::core::SchemaBrokenReferenceError &error) {
          std::cerr << "error: Could not autofix the schema without breaking "
                       "its internal references\n";
          std::cerr << "  at " << entry.first.string() << "\n";
          std::cerr << "  at schema location \""
                    << sourcemeta::core::to_string(error.location())
                    << "\"\n\n";

          std::cerr << "We are working hard to improve the autofixing "
                       "functionality to re-phrase\n";
          std::cerr << "references in all possible edge cases\n\n";
          std::cerr << "For now, try again without `--fix/-f` and applying "
                       "the suggestions by hand\n\n";
          std::cerr << "Also consider consider reporting this problematic case "
                       "to the issue tracker:\n";
          std::cerr << "https://github.com/sourcemeta/jsonschema/issues\n";
          return EXIT_FAILURE;
        } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
          throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
              entry.first);
        } catch (const sourcemeta::core::SchemaResolutionError &error) {
          throw FileError<sourcemeta::core::SchemaResolutionError>(entry.first,
                                                                   error);
        }
      });

      if (wrapper_result == EXIT_SUCCESS) {
        if (copy != entry.second) {
          std::ofstream output{entry.first};
          sourcemeta::core::prettify(copy, output);
          output << "\n";
        }
      } else {
        result = false;
      }
    }
  } else {
    for (const auto &entry :
         for_each_json(options.positional(), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";

      const auto wrapper_result = sourcemeta::jsonschema::try_catch([&]() {
        try {
          const auto subresult = bundle.check(
              entry.second, sourcemeta::core::schema_official_walker,
              custom_resolver,
              get_lint_callback(errors_array, entry, output_json), dialect,
              sourcemeta::core::URI::from_path(entry.first).recompose());
          scores.emplace_back(subresult.second);
          if (subresult.first) {
            return EXIT_SUCCESS;
          } else {
            return EXIT_FAILURE;
          }
        } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
          throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
              entry.first);
        } catch (const sourcemeta::core::SchemaResolutionError &error) {
          throw FileError<sourcemeta::core::SchemaResolutionError>(entry.first,
                                                                   error);
        }
      });

      if (wrapper_result != EXIT_SUCCESS) {
        result = false;
      }
    }
  }

  if (output_json) {
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
    sourcemeta::core::prettify(output_json_object, std::cout);
    std::cout << "\n";
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
