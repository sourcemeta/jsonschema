#include <cstdlib>     // EXIT_SUCCESS
#include <iostream>    // std::cout
#include <string_view> // std::string_view

#include "command.h"

constexpr std::string_view USAGE_DETAILS{R"EOF(
COMMANDS

   version                                   Print version information and quit

   help                                      Print this help information and quit

   validate <schema.json> <instance.json>    Validate an instance against a schema

   fmt <path/to/schema.json>                 Format JSON Schema in-place

   lint <path/to/schema.json>                Lint JSON Schema

   frame <path/to/schema.json>               Frame a JSON Schema in-place

   compile <path/to/schema.json>             Compile a JSON Schema for efficient evaluation
                                             and print the result to standard output

)EOF"};

auto intelligence::jsonschema::cli::help(const std::string &program) -> int {
  std::cout << "Usage: " << program << " <command> [arguments...]\n";
  std::cout << USAGE_DETAILS;
  return EXIT_SUCCESS;
}
