#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>

#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <filesystem>  // std::filesystem
#include <iostream>    // std::cerr, std::cout
#include <string>      // std::string
#include <string_view> // std::string_view

#include "command.h"
#include "configure.h"
#include "error.h"
#include "utils.h"

constexpr std::string_view USAGE_DETAILS{R"EOF(
Global Options:

   --verbose, -v                  Enable verbose output
   --resolve, -r                  Import the given JSON Schema (or directory of schemas)
                                  into the resolution context
   --default-dialect, -d <uri>    Specify the URI for the default dialect to be used
                                  if the `$schema` keyword is not set
   --json, -j                     Prefer JSON output if supported

Commands:

   version / --version / -v

       Print the current version of the JSON Schema CLI.

   help / --help / -h

       Print this command reference help.

   validate <schema.json|.yaml> <instance.json|.jsonl|.yaml...> [--http/-h]
            [--benchmark/-b] [--loop <iterations>]
            [--extension/-e <extension>]
            [--ignore/-i <schemas-or-directories>] [--trace/-t] [--fast/-f]
            [--template/-m <template.json>]

       Validate one or more instances against the given schema.

       By default, schemas are validated in exhaustive mode, which results in
       better error messages, at the expense of speed. The --fast/-f option
       makes the schema compiler optimise for speed, at the expense of error
       messages. Looping in benchmark mode allows to collect the execution time
       average and standard deviation over multiple runs.

       You may additionally pass a pre-compiled schema template (see the
       `compile` command). However, you still need to pass the original schema
       for error reporting purposes. Make sure they match or you will get
       non-sense results.

   metaschema [schemas-or-directories...] [--http/-h]
              [--extension/-e <extension>]
              [--ignore/-i <schemas-or-directories>] [--trace/-t]

       Validate that a schema or a set of schemas are valid with respect
       to their metaschemas.

   compile <schema.json|.yaml> [--http/-h] [--extension/-e <extension>]
           [--ignore/-i <schemas-or-directories>] [--fast/-f] [--minify/-m]

       Compile the given schema into an internal optimised representation.

   test [schemas-or-directories...] [--http/-h] [--extension/-e <extension>]
        [--ignore/-i <schemas-or-directories>]

       Run a set of unit tests against a schema.

   fmt [schemas-or-directories...] [--check/-c] [--extension/-e <extension>]
       [--ignore/-i <schemas-or-directories>] [--keep-ordering/-k]
       [--indentation/-n <spaces>]

       Format the input schemas in-place or check they are formatted.
       This command does not support YAML schemas yet.

   lint [schemas-or-directories...] [--fix/-f] [--extension/-e <extension>]
        [--ignore/-i <schemas-or-directories>] [--exclude/-x <rule-name>]
        [--only/-o <rule-name>] [--list/-l] [--strict/-s]
        [--indentation/-n <spaces>]

       Lint the input schemas and potentially fix the reported issues.
       The --fix/-f option is not supported when passing YAML schemas.
       Use --list/-l to print a summary of all enabled rules.
       Use --strict/-s to enable additional opinionated strict rules.
       Use --indentation/-n to keep indentation when auto-fixing

   bundle <schema.json|.yaml> [--http/-h] [--extension/-e <extension>]
          [--ignore/-i <schemas-or-directories>] [--without-id/-w]

       Perform JSON Schema Bundling on a schema to inline remote references,
       printing the result to standard output.

   inspect <schema.json|.yaml>

       Statically inspect a schema to display schema locations and
       references in a human-readable manner.

   encode <document.json|.jsonl> <output.binpack>

       Encode a JSON document or JSONL dataset using JSON BinPack.

   decode <output.binpack> <output.json|.jsonl>

       Decode a JSON document or JSONL dataset using JSON BinPack.

For more documentation, visit https://github.com/sourcemeta/jsonschema
)EOF"};

auto jsonschema_main(const std::string &program, const std::string &command,
                     sourcemeta::core::Options &app, int argc, char *argv[])
    -> int {
  if (command == "fmt") {
    app.flag("check", {"c"});
    app.flag("keep-ordering", {"k"});
    app.option("extension", {"e"});
    app.option("ignore", {"i"});
    app.option("indentation", {"n"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::fmt(app);
    return EXIT_SUCCESS;
  } else if (command == "inspect") {
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::inspect(app);
    return EXIT_SUCCESS;
  } else if (command == "bundle") {
    app.flag("without-id", {"w"});
    app.option("extension", {"e"});
    app.option("ignore", {"i"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::bundle(app);
    return EXIT_SUCCESS;
  } else if (command == "lint") {
    app.flag("fix", {"f"});
    app.flag("list", {"l"});
    app.flag("strict", {"s"});
    app.option("extension", {"e"});
    app.option("exclude", {"x"});
    app.option("only", {"o"});
    app.option("ignore", {"i"});
    app.option("indentation", {"n"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::lint(app);
    return EXIT_SUCCESS;
  } else if (command == "validate") {
    app.flag("benchmark", {"b"});
    app.flag("trace", {"t"});
    app.flag("fast", {"f"});
    app.option("extension", {"e"});
    app.option("template", {"m"});
    app.option("loop", {"l"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::validate(app);
    return EXIT_SUCCESS;
  } else if (command == "metaschema") {
    app.flag("trace", {"t"});
    app.option("extension", {"e"});
    app.option("ignore", {"i"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::metaschema(app);
    return EXIT_SUCCESS;
  } else if (command == "compile") {
    app.flag("fast", {"f"});
    app.flag("minify", {"m"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::compile(app);
    return EXIT_SUCCESS;
  } else if (command == "test") {
    app.option("extension", {"e"});
    app.option("ignore", {"i"});
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::test(app);
    return EXIT_SUCCESS;
  } else if (command == "encode") {
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::encode(app);
    return EXIT_SUCCESS;
  } else if (command == "decode") {
    app.parse(argc, argv, {.skip = 1});
    sourcemeta::jsonschema::decode(app);
    return EXIT_SUCCESS;
  } else if (command == "help" || command == "--help" || command == "-h") {
    std::cout << "JSON Schema CLI - v"
              << sourcemeta::jsonschema::PROJECT_VERSION << "\n";
    std::cout << "Usage: " << std::filesystem::path{program}.filename().string()
              << " <command> [arguments...]\n";
    std::cout << USAGE_DETAILS;
    return EXIT_SUCCESS;
  } else if (command == "version" || command == "--version" ||
             command == "-v") {
    std::cout << sourcemeta::jsonschema::PROJECT_VERSION << "\n";
    return EXIT_SUCCESS;
  } else {
    throw sourcemeta::jsonschema::UnknownCommandError{command};
  }
}

auto main(int argc, char *argv[]) noexcept -> int {
  sourcemeta::core::Options app;
  app.flag("http", {"h"});
  app.flag("verbose", {"v"});
  app.flag("json", {"j"});
  app.option("resolve", {"r"});
  app.option("default-dialect", {"d"});

  return sourcemeta::jsonschema::try_catch(app, [&app, argc, &argv]() {
    const std::string program{argv[0]};
    const std::string command{argc > 1 ? argv[1] : "help"};
    return jsonschema_main(program, command, app, argc, argv);
  });
}
