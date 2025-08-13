#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/linter.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout
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

static auto get_lint_callback(sourcemeta::core::JSON &errors_array,
                              const std::filesystem::path &path,
                              const bool output_json) -> auto {
  return [&path, &errors_array,
          output_json](const auto &pointer, const auto &name,
                       const auto &message, const auto &description) {
    if (output_json) {
      auto error_obj = sourcemeta::core::JSON::make_object();

      error_obj.assign("path", sourcemeta::core::JSON{path.string()});
      error_obj.assign("id", sourcemeta::core::JSON{name});
      error_obj.assign("message", sourcemeta::core::JSON{message});

      if (description.empty()) {
        error_obj.assign("description", sourcemeta::core::JSON{nullptr});
      } else {
        error_obj.assign("description", sourcemeta::core::JSON{message});
      }

      std::ostringstream pointer_stream;
      sourcemeta::core::stringify(pointer, pointer_stream);
      error_obj.assign("schemaLocation",
                       sourcemeta::core::JSON{pointer_stream.str()});

      errors_array.push_back(error_obj);
    } else {
      std::cout << path.string() << ":\n";
      std::cout << "  " << message << " (" << name << ")\n";
      std::cout << "    at schema location \"";
      sourcemeta::core::stringify(pointer, std::cout);
      std::cout << "\"\n";
      if (!description.empty()) {
        reindent(description, "    ", std::cout);
        if (description.back() != '\n') {
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
  sourcemeta::core::add(bundle, sourcemeta::core::AlterSchemaMode::Readability);

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
              get_lint_callback(errors_array, entry.first, output_json),
              dialect,
              sourcemeta::core::URI::from_path(entry.first).recompose());
          return EXIT_SUCCESS;
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

      if (copy != entry.second) {
        std::ofstream output{entry.first};
        sourcemeta::core::prettify(copy, output);
        output << "\n";
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
              get_lint_callback(errors_array, entry.first, output_json),
              dialect,
              sourcemeta::core::URI::from_path(entry.first).recompose());
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
    output_json_object.assign("errors", sourcemeta::core::JSON{errors_array});
    sourcemeta::core::prettify(output_json_object, std::cout);
    std::cout << "\n";
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
