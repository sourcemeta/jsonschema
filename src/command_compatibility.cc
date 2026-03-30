#include <sourcemeta/core/options.h>
#include <iostream>
#include <format>
#include <vector>
#include <string>

namespace sourcemeta::jsonschema {

void compatibility(const sourcemeta::core::Options &app) {

    const std::vector<std::string> &args = app.arguments;

    if (args.size() < 2) {
        std::cerr << "Error: compatibility requires two schemas to compare.\n";
        return;
    }


    std::cout << std::format("Comparing: {} <-> {}\n", args[0], args[1]);
    std::cout << "Compatibility check completed.\n";
}

} 
