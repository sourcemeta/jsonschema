#include <algorithm>   // std::min
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <filesystem>  // std::filesystem
#include <iostream>    // std::cerr, std::cout
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

#include "command.h"
#include "configure.h"

constexpr std::string_view USAGE_DETAILS{R"EOF(
Global Options:

   --verbose, -v    Enable verbose output

Commands:

   validate <schema.json> <instance.json>

       Validate an instance against a schema, printing error information, if
       any, in a human-readable manner.

   test [schema.json...]

       A schema test runner inspired by the official JSON Schema test suite.
       Passing directories as input will run every `.json` file in such
       directory (recursively) as a test. If no argument is passed, run every
       `.json` file in the current working directory (recursively) as a test.

   fmt [schema.json...] [--check/-c]

       Format the input schemas in-place. Passing directories as input means
       to format every `.json` file in such directory (recursively). If no
       argument is passed, format every `.json` file in the current working
       directory (recursively). The `--check/-c` option will check if the given
       schemas adhere to the desired formatting without modifying them.

   lint [schema.json...] [--fix/-f]

       Lint the input schemas. Passing directories as input means to lint
       every `.json` file in such directory (recursively). If no argument is
       passed, lint every `.json` file in the current working directory
       (recursively). The `--fix/-f` option will attempt to automatically
       fix the linter errors.

   bundle <schema.json>

       Perform JSON Schema Bundling on a schema to inline remote references,
       printing the result to standard output. Read
       https://json-schema.org/blog/posts/bundling-json-schema-compound-documents
       to learn more.

   frame <schema.json>

       Frame a schema in-place, displaying schema locations and references
       in a human-readable manner.

)EOF"};

auto jsonschema_main(const std::string &program, const std::string &command,
                     const std::span<const std::string> &arguments) -> int {
  if (command == "fmt") {
    return intelligence::jsonschema::cli::fmt(arguments);
  } else if (command == "frame") {
    return intelligence::jsonschema::cli::frame(arguments);
  } else if (command == "bundle") {
    return intelligence::jsonschema::cli::bundle(arguments);
  } else if (command == "lint") {
    return intelligence::jsonschema::cli::lint(arguments);
  } else if (command == "validate") {
    return intelligence::jsonschema::cli::validate(arguments);
  } else if (command == "test") {
    return intelligence::jsonschema::cli::test(arguments);
  } else {
    std::cout << "JSON Schema CLI - v"
              << intelligence::jsonschema::cli::PROJECT_VERSION << "\n";
    std::cout << "Usage: " << std::filesystem::path{program}.filename().string()
              << " <command> [arguments...]\n";
    std::cout << USAGE_DETAILS;
    return EXIT_SUCCESS;
  }
}

auto main(int argc, char *argv[]) noexcept -> int {
  try {
    const std::string program{argv[0]};
    const std::string command{argc > 1 ? argv[1] : "help"};
    const std::vector<std::string> arguments{argv + std::min(2, argc),
                                             argv + argc};
    return jsonschema_main(program, command, arguments);
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
}
