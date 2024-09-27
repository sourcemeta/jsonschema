#include "commands.h"

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cerr

static const char *USAGE_DETAILS = R"EOF(
   version                           Print version information and quit.

   help                              Print this help information and quit.

   canonicalize [schema.json]        Canonicalize a given JSON Schema definition
                                     and print the result to stdout. If a path to
                                     a schema is not provided, the schema will
                                     be read from standard input.

   compile [schema.json]             Compile a given JSON Schema definition
                                     into an encoding schema and print the result to
                                     stdout. If a path to a schema is not provided,
                                     the schema will be read from standard input.

   encode <schema.json> [data.json]  Encode a given JSON instance based on the given
                                     encoding schema (or JSON Schema) and print the
                                     results to stdout. If a path to the instance is
                                     not provided, the instance will be read from
                                     standard input.

   decode <schema.json> [data.bin]   Decode a given bit-string based on the given
                                     encoding schema (or JSON Schema) and print the
                                     results to stdout.  If a path to the bit-string
                                     is not provided, it will be read from standard
                                     input.
)EOF";

auto sourcemeta::jsonbinpack::cli::help(const std::string &program) -> int {
  std::clog << "Usage: " << program << " <command> [arguments...]\n";
  std::clog << USAGE_DETAILS;
  return EXIT_SUCCESS;
}
