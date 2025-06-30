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
#include "utils.h"

constexpr std::string_view USAGE_DETAILS{R"EOF(
Global Options:

   --verbose, -v                  Enable verbose output
   --resolve, -r                  Import the given JSON Schema (or directory of schemas)
                                  into the resolution context
   --default-dialect, -d <uri>    Specify the URI for the default dialect to be used
                                  if the `$schema` keyword is not set

Commands:

   version / --version

       Print the current version of the JSON Schema CLI.

   help / --help / -h

       Print this command reference help.

   validate <schema.json|.yaml> <instance.json|.jsonl|.yaml...> [--http/-h]
            [--benchmark/-b] [--extension/-e <extension>]
            [--ignore/-i <schemas-or-directories>] [--trace/-t] [--fast/-f]
            [--template/-m <template.json>] [--json/-j]

       Validate one or more instances against the given schema.

       By default, schemas are validated in exhaustive mode, which results in
       better error messages, at the expense of speed. The --fast/-f option
       makes the schema compiler optimise for speed, at the expense of error
       messages.

       You may additionally pass a pre-compiled schema template (see the
       `compile` command). However, you still need to pass the original schema
       for error reporting purposes. Make sure they match or you will get
       non-sense results.

   metaschema [schemas-or-directories...] [--http/-h]
              [--extension/-e <extension>]
              [--ignore/-i <schemas-or-directories>] [--trace/-t] [--json/-j]

       Validate that a schema or a set of schemas are valid with respect
       to their metaschemas.

   compile <schema.json|.yaml> [--http/-h] [--extension/-e <extension>]
           [--ignore/-i <schemas-or-directories>] [--fast/-f]

       Compile the given schema into an internal optimised representation.

   test [schemas-or-directories...] [--http/-h] [--extension/-e <extension>]
        [--ignore/-i <schemas-or-directories>]

       Run a set of unit tests against a schema.

   fmt [schemas-or-directories...] [--check/-c] [--extension/-e <extension>]
       [--ignore/-i <schemas-or-directories>] [--keep-ordering/-k]

       Format the input schemas in-place or check they are formatted.
       This command does not support YAML schemas yet.

   lint [schemas-or-directories...] [--fix/-f] [--json/-j]
        [--extension/-e <extension>] [--ignore/-i <schemas-or-directories>]
        [--exclude/-x <rule-name>] [--keep-ordering/-k]

       Lint the input schemas and potentially fix the reported issues.
       The --fix/-f option is not supported when passing YAML schemas.
       Use --json/-j to output lint errors in JSON.
       The --keep-ordering/-k option is only used when --fix/-f is
       also present.

   bundle <schema.json|.yaml> [--http/-h] [--extension/-e <extension>]
          [--ignore/-i <schemas-or-directories>] [--without-id/-w]

       Perform JSON Schema Bundling on a schema to inline remote references,
       printing the result to standard output.

   inspect <schema.json|.yaml> [--json/-j]

       Statically inspect a schema to display schema locations and
       references in a human-readable manner.
       Use --json/-j to output information in JSON.

   encode <document.json|.jsonl> <output.binpack>

       Encode a JSON document or JSONL dataset using JSON BinPack.

   decode <output.binpack> <output.json|.jsonl>

       Decode a JSON document or JSONL dataset using JSON BinPack.

For more documentation, visit https://github.com/sourcemeta/jsonschema
)EOF"};

auto jsonschema_main(const std::string &program, const std::string &command,
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
  } else if (command == "compile") {
    return sourcemeta::jsonschema::cli::compile(arguments);
  } else if (command == "test") {
    return sourcemeta::jsonschema::cli::test(arguments);
  } else if (command == "encode") {
    return sourcemeta::jsonschema::cli::encode(arguments);
  } else if (command == "decode") {
    return sourcemeta::jsonschema::cli::decode(arguments);
  } else if (command == "help" || command == "--help" || command == "-h") {
    std::cout << "JSON Schema CLI - v"
              << sourcemeta::jsonschema::cli::PROJECT_VERSION << "\n";
    std::cout << "Usage: " << std::filesystem::path{program}.filename().string()
              << " <command> [arguments...]\n";
    std::cout << USAGE_DETAILS;
    return EXIT_SUCCESS;
  } else if (command == "version" || command == "--version") {
    std::cout << sourcemeta::jsonschema::cli::PROJECT_VERSION << "\n";
    return EXIT_SUCCESS;
  } else {
    std::cerr << "error: Unknown command '" << command << "'\n";
    std::cerr << "Use '--help' for usage information\n";
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
    std::cerr << "error: " << error.what() << "\n  " << error.id() << "\n";
    std::cerr << "\nThis is likely because you forgot to import such schema "
                 "using --resolve/-r\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownDialectError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr
        << "\nThis is likely because you forgot to import such meta-schema "
           "using --resolve/-r\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::cli::FileError<
           sourcemeta::core::SchemaUnknownBaseDialectError> &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "  "
              << sourcemeta::jsonschema::cli::safe_weakly_canonical(
                     error.path())
                     .string()
              << "\n";
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr << "If the input does not declare the $schema keyword, you might "
                 "want to\n";
    std::cerr
        << "explicitly declare a default dialect using --default-dialect/-d\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr << "If the input does not declare the $schema keyword, you might "
                 "want to\n";
    std::cerr
        << "explicitly declare a default dialect using --default-dialect/-d\n";
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
              << sourcemeta::jsonschema::cli::safe_weakly_canonical(
                     error.path())
                     .string()
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
                << sourcemeta::jsonschema::cli::safe_weakly_canonical(
                       error.path1())
                       .string()
                << "\n";
    } else if (error.code() == std::errc::is_a_directory) {
      std::cerr << "error: The input was supposed to be a file but it is a "
                   "directory\n  "
                << sourcemeta::jsonschema::cli::safe_weakly_canonical(
                       error.path1())
                       .string()
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
