#include "commands.h"
#include "version.h"

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout

auto sourcemeta::jsonbinpack::cli::version() -> int {
  std::cout << sourcemeta::jsonbinpack::VERSION << "\n";
  return EXIT_SUCCESS;
}
