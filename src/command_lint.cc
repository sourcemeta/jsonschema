#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::lint(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"f", "fix", "json", "j"})};
  const bool output_json = options.contains("json") || options.contains("j");

  sourcemeta::core::SchemaTransformer bundle;
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::AntiPattern);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Simplify);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Superfluous);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Redundant);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::SyntaxSugar);

  bool result{true};

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
      bundle.apply(copy, sourcemeta::core::schema_official_walker,
                   resolver(options));
      std::ofstream output{entry.first};
      sourcemeta::core::prettify(copy, output,
                                 sourcemeta::core::schema_format_compare);
      output << "\n";
    }

    // If --fix and --json were both requested, optionally print a small JSON:
    if (output_json) {
      auto msg = sourcemeta::core::JSON::make_object();
      msg.assign("fixApplied", sourcemeta::core::JSON{true});
      sourcemeta::core::prettify(msg, std::cout);
      std::cout << std::endl;
    }
  } else {
    auto issues_array = sourcemeta::core::JSON::make_array();
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      const bool subresult = bundle.check(
          entry.second, sourcemeta::core::schema_official_walker,
          resolver(options),
          [&](const auto &pointer, const auto &name, const auto &message) {
            if (output_json) {
              // Collect in a JSON object instead of printing lines
              auto error_obj = sourcemeta::core::JSON::make_object();

              error_obj.assign("file",
                               sourcemeta::core::JSON{entry.first.string()});
              error_obj.assign("rule", sourcemeta::core::JSON{name});
              error_obj.assign("message", sourcemeta::core::JSON{message});

              std::ostringstream pointer_stream;
              sourcemeta::core::stringify(pointer, pointer_stream);
              error_obj.assign("pointer",
                               sourcemeta::core::JSON{pointer_stream.str()});

              issues_array.push_back(error_obj);
            } else {
              std::cout << entry.first.string() << ":\n";
              std::cout << "  " << message << " (" << name << ")\n";
              std::cout << "    at schema location \"";
              sourcemeta::core::stringify(pointer, std::cout);
              std::cout << "\"\n";
            }
          });

      if (subresult) {
        log_verbose(options) << "PASS: " << entry.first.string() << "\n";
      } else {
        result = false;
      }
    }
    if (output_json) {
      auto output_json_object = sourcemeta::core::JSON::make_object();
      output_json_object.assign("passed", sourcemeta::core::JSON{result});
      output_json_object.assign("issues", sourcemeta::core::JSON{issues_array});
      sourcemeta::core::prettify(output_json_object, std::cout);
      std::cout << std::endl;
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
