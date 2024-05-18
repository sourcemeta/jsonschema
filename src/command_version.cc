#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout

#include "command.h"
#include "configure.h"

auto intelligence::jsonschema::cli::version() -> int {
  std::cout << PROJECT_VERSION << "\n";
  return EXIT_SUCCESS;
}
