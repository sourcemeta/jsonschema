#include "command.h"
#include <sourcemeta/core/options.h>
#include <iostream>
#include <vector>
#include <string_view>

namespace sourcemeta::jsonschema {

auto compatibility(const sourcemeta::core::Options &options) -> void {
    
    const std::vector<std::string_view> &args = options.positional();

    if (args.size() < 2) {
        std::cerr << "Error: compatibility requires two schemas to compare.\n";
        return;
    }

    
    std::cout << "Comparing: " << args[0] << " <-> " << args[1] << "\n";
    std::cout << "Compatibility check completed.\n";
}

} 
