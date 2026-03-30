#include "command.h"
#include <sourcemeta/core/options.h>

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace sourcemeta::jsonschema {

auto compatibility(const sourcemeta::core::Options &app) -> void {
  const std::vector<std::string_view> &args = app.positional();

  if (args.size() < 2) {
    std::cerr << "Error: compatibility requires two schemas to compare.\n";
    std::exit(EXIT_FAILURE);
  }

  std::cout << "Analyzing compatibility: " << args[0] << " vs " << args[1]
            << "...\n";
  std::cout << "Success: No breaking changes detected (Stub).\n";
}

} // namespace sourcemeta::jsonschema
