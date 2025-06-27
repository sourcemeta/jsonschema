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
#include "utils.h"

template <typename Options, typename Iterator>
static auto disable_lint_rules(sourcemeta::core::SchemaTransformer &bundle,
                               const Options &options, Iterator first,
                               Iterator last) -> void {
  for (auto iterator = first; iterator != last; ++iterator) {
    if (bundle.remove(*iterator)) {
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

auto sourcemeta::jsonschema::cli::lint(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(
      arguments, {"f", "fix", "json", "j", "k", "keep-ordering"})};
  const bool output_json = options.contains("json") || options.contains("j");

  sourcemeta::core::SchemaTransformer bundle;
  sourcemeta::core::add(bundle, sourcemeta::core::AlterSchemaMode::Readability);

  bundle.add<sourcemeta::blaze::ValidExamples>(
      sourcemeta::blaze::default_schema_compiler);
  bundle.add<sourcemeta::blaze::ValidDefault>(
      sourcemeta::blaze::default_schema_compiler);

  if (options.contains("exclude")) {
    disable_lint_rules(bundle, options, options.at("exclude").cbegin(),
                       options.at("exclude").cend());
  }

  if (options.contains("x")) {
    disable_lint_rules(bundle, options, options.at("x").cbegin(),
                       options.at("x").cend());
  }

  bool result{true};
  auto errors_array = sourcemeta::core::JSON::make_array();
  const auto dialect{default_dialect(options)};

  if (options.contains("f") || options.contains("fix")) {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      if (entry.first.extension() == ".yaml" ||
          entry.first.extension() == ".yml") {
        std::cerr << "The --fix option is not supported for YAML input files\n";
        return EXIT_FAILURE;
      }

      auto copy = entry.second;

      try {
        bundle.apply(
            copy, sourcemeta::core::schema_official_walker,
            resolver(options, options.contains("h") || options.contains("http"),
                     dialect),
            get_lint_callback(errors_array, entry.first, output_json), dialect,
            sourcemeta::core::URI::from_path(entry.first).recompose());
      } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
        throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
            entry.first);
      }

      std::ofstream output{entry.first};
      if (options.contains("k") || options.contains("keep-ordering")) {
        sourcemeta::core::prettify(copy, output);
      } else {
        sourcemeta::core::prettify(copy, output,
                                   sourcemeta::core::schema_format_compare);
      }
      output << "\n";
    }
  } else {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      try {
        const bool subresult = bundle.check(
            entry.second, sourcemeta::core::schema_official_walker,
            resolver(options, options.contains("h") || options.contains("http"),
                     dialect),
            get_lint_callback(errors_array, entry.first, output_json), dialect,
            sourcemeta::core::URI::from_path(entry.first).recompose());
        if (!subresult) {
          result = false;
        }
      } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
        throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
            entry.first);
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
