#include <sourcemeta/core/jsonschema.h>

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

   validate <schema.json|.yaml> <instance.json|.jsonl|.yaml...> [--http/-h]
            [--benchmark/-b] [--extension/-e <extension>]
            [--ignore/-i <schemas-or-directories>] [--trace/-t]

       Validate one or more instances against the given schema.

   metaschema [schemas-or-directories...] [--http/-h]
              [--extension/-e <extension>]
              [--ignore/-i <schemas-or-directories>] [--trace/-t]

       Validate that a schema or a set of schemas are valid with respect
       to their metaschemas.

   test [schemas-or-directories...] [--http/-h] [--extension/-e <extension>]
        [--ignore/-i <schemas-or-directories>]

       Run a set of unit tests against a schema.

   fmt [schemas-or-directories...] [--check/-c] [--extension/-e <extension>]
       [--ignore/-i <schemas-or-directories>]

       Format the input schemas in-place or check they are formatted.
       This command does not support YAML schemas yet.

   lint [schemas-or-directories...] [--fix/-f] [--extension/-e <extension>]
        [--ignore/-i <schemas-or-directories>]

       Lint the input schemas and potentially fix the reported issues.
       The --fix/-f option is not supported when passing YAML schemas.

   bundle <schema.json|.yaml> [--http/-h] [--extension/-e <extension>]
          [--ignore/-i <schemas-or-directories>] [--without-id/-w]

       Perform JSON Schema Bundling on a schema to inline remote references,
       printing the result to standard output.

   inspect <schema.json|.yaml> [--json/-j]

       Statically inspect a schema to display schema locations and
       references in a human-readable manner.

   encode <document.json|.jsonl> <output.binpack>

       Encode a JSON document or JSONL dataset using JSON BinPack.

   decode <output.binpack> <output.json|.jsonl>

       Decode a JSON document or JSONL dataset using JSON BinPack.

For more documentation, visit https://github.com/sourcemeta/jsonschema
)EOF"};

auto jsonschema_main([[maybe_unused]] const std::string &program,
                     const std::string &command,
                     const std::span<const std::string> &arguments) -> int {
  if (command == "fmt") {
    return sourcemeta::jsonschema::cli::fmt(arguments);
  } else if (command == "inspect") {
    return sourcemeta::jsonschema::cli::inspect(arguments);
  } else if (command == "bundle") {
    return sourcemeta::jsonschema::cli::bundle(arguments);
  } else if (command == "lint") {
    return sourcemeta::jsonschema::cli::lint(arguments);
  } else if (command == "validate") {
    return sourcemeta::jsonschema::cli::validate(arguments);
  } else if (command == "metaschema") {
    return sourcemeta::jsonschema::cli::metaschema(arguments);
  } else if (command == "test") {
    return sourcemeta::jsonschema::cli::test(arguments);
  } else if (command == "encode") {
    return sourcemeta::jsonschema::cli::encode(arguments);
  } else if (command == "decode") {
    return sourcemeta::jsonschema::cli::decode(arguments);
  } else if (command == "help") {
    std::cout << "JSON Schema CLI - v"
              << sourcemeta::jsonschema::cli::PROJECT_VERSION << "\n";
    std::cout << "Usage: " << std::filesystem::path{program}.filename().string()
              << " <command> [arguments...]\n";
    std::cout << USAGE_DETAILS;
    return EXIT_SUCCESS;
  } else {
    std::cerr << "error: Unknown command '" << command << "'.\n";
    std::cerr << "Use '--help' for usage information.\n";
    return EXIT_FAILURE;
  }
}

auto main(int argc, char *argv[]) noexcept -> int {
  try {
    const std::string program{argv[0]};
    const std::string command{argc > 1 ? argv[1] : "help"};
    const std::vector<std::string> arguments{argv + std::min(2, argc),
                                             argv + argc};
    return jsonschema_main(program, command, arguments);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.id()
              << "\n    at schema location \"";
    sourcemeta::core::stringify(error.location(), std::cerr);
    std::cerr << "\"\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    std::cerr << "error: " << error.what() << "\n  at " << error.id() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaError &error) {
    std::cerr << "error: " << error.what() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.uri()
              << "\n\nTo request support for it, please open an issue "
                 "at\nhttps://github.com/sourcemeta/jsonschema\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::URIParseError &error) {
    std::cerr << "error: " << error.what() << " at column " << error.column()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONFileParseError &error) {
    std::cerr << "error: " << error.what() << " at line " << error.line()
              << " and column " << error.column() << "\n  "
              << std::filesystem::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONParseError &error) {
    std::cerr << "error: " << error.what() << " at line " << error.line()
              << " and column " << error.column() << "\n";
    return EXIT_FAILURE;
  } catch (const std::filesystem::filesystem_error &error) {
    // See https://en.cppreference.com/w/cpp/error/errc
    if (error.code() == std::errc::no_such_file_or_directory) {
      std::cerr << "error: " << error.code().message() << "\n  "
                << std::filesystem::weakly_canonical(error.path1()).string()
                << "\n";
    } else if (error.code() == std::errc::is_a_directory) {
      std::cerr << "error: The input was supposed to be a file but it is a "
                   "directory\n  "
                << std::filesystem::weakly_canonical(error.path1()).string()
                << "\n";
    } else {
      std::cerr << "error: " << error.what() << "\n";
    }

    return EXIT_FAILURE;
  } catch (const std::runtime_error &error) {
    std::cerr << "error: " << error.what() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &error) {
    std::cerr << "unexpected error: " << error.what()
              << "\nPlease report it at "
              << "https://github.com/sourcemeta/jsonschema\n";
    return EXIT_FAILURE;
  }
}
