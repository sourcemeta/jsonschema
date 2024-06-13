#include <sourcemeta/jsontoolkit/jsonschema.h>

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
   --resolve, -r    Import the given JSON Schema (or directory of schemas)
                    into the resolution context

Commands:

   validate <schema.json> [instance.json] [--http/-h] [--metaschema/-m]

       If an instance is passed, validate it against the given schema.
       Otherwise, validate the schema against its dialect metaschema. The
       `--http/-h` option enables resolving remote schemas over the HTTP
       protocol. The `--metaschema/-m` option checks that the given schema
       is valid with respects to its dialect metaschema even if an instance
       was passed.

   test [schemas-or-directories...] [--http/-h] [--metaschema/-m]
        [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]

       A schema test runner inspired by the official JSON Schema test suite.
       Passing directories as input will run every `.json` file in such
       directory (recursively) as a test. If no argument is passed, run every
       `.json` file in the current working directory (recursively) as a test.
       The `--ignore/-i` option can be set to files or directories to ignore.

       The `--http/-h` option enables resolving remote schemas over the HTTP
       protocol. The `--metaschema/-m` option checks that the given schema is
       valid with respects to its dialect metaschema. When scanning
       directories, the `--extension/-e` option is used to prefer a file
       extension other than `.json`. This option can be set multiple times.

   fmt [schemas-or-directories...] [--check/-c] [--extension/-e <extension>]
       [--ignore/-i <schemas-or-directories>]

       Format the input schemas in-place. Passing directories as input means
       to format every `.json` file in such directory (recursively). If no
       argument is passed, format every `.json` file in the current working
       directory (recursively). The `--ignore/-i` option can be set to files
       or directories to ignore.

       The `--check/-c` option will check if the given
       schemas adhere to the desired formatting without modifying them. When
       scanning directories, the `--extension/-e` option is used to prefer a
       file extension other than `.json`. This option can be set multiple times.

   lint [schemas-or-directories...] [--fix/-f] [--extension/-e <extension>]
       [--ignore/-i <schemas-or-directories>]

       Lint the input schemas. Passing directories as input means to lint
       every `.json` file in such directory (recursively). If no argument is
       passed, lint every `.json` file in the current working directory
       (recursively). The `--ignore/-i` option can be set to files or
       directories to ignore.

       The `--fix/-f` option will attempt to automatically
       fix the linter errors. When scanning directories, the `--extension/-e`
       option is used to prefer a file extension other than `.json`. This option
       can be set multiple times.

   bundle <schema.json> [--http/-h]

       Perform JSON Schema Bundling on a schema to inline remote references,
       printing the result to standard output. Read
       https://json-schema.org/blog/posts/bundling-json-schema-compound-documents
       to learn more. The `--http/-h` option enables resolving remote schemas
       over the HTTP protocol.

   frame <schema.json>

       Frame a schema in-place, displaying schema locations and references
       in a human-readable manner.

For more documentation, visit https://github.com/Intelligence-AI/jsonschema
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
  } catch (const sourcemeta::jsontoolkit::SchemaReferenceError &error) {
    std::cerr << error.what() << ": " << error.id() << "\n";
    std::cerr << "    at schema location \"";
    sourcemeta::jsontoolkit::stringify(error.location(), std::cerr);
    std::cerr << "\"\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsontoolkit::SchemaResolutionError &error) {
    std::cerr << error.what() << ": " << error.id() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
}
