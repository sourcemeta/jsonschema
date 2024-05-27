#include <cstdlib>     // EXIT_SUCCESS
#include <filesystem>  // std::filesystem
#include <iostream>    // std::cout
#include <string_view> // std::string_view

#include "command.h"
#include "configure.h"

constexpr std::string_view USAGE_DETAILS{R"EOF(
Commands:

   validate <schema.json> <instance.json>

       Validate an instance against a schema, printing error information, if
       any, in a human-readable manner.

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

   frame <schema.json>

       Frame a schema in-place, displaying schema locations and references
       in a human-readable manner.

   help

       Print this help information to standard output and quit.

)EOF"};

auto intelligence::jsonschema::cli::help(const std::string &program) -> int {
  std::cout << "JSON Schema CLI - v" << PROJECT_VERSION << "\n";
  std::cout << "Usage: " << std::filesystem::path{program}.filename().string()
            << " <command> [arguments...]\n";
  std::cout << USAGE_DETAILS;
  return EXIT_SUCCESS;
}
