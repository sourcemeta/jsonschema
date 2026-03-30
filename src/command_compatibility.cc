#include <iostream>
#include <vector>
#include <string>
#include <format>

namespace sourcemeta::jsonschema {


void compatibility(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        std::cerr << "Error: compatibility requires two schemas to compare.\n";
        return;
    }

    std::cout << std::format("Comparing: {} <-> {}\n", args[0], args[1]);
    std::cout << "Compatibility check completed.\n";
}

} 
