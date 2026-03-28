#include "command.h"
#include <iostream>
#include <format> // C++20 

namespace sourcemeta::jsonschema {

void compatibility(const Command &app) {

    if (args.size() < 2) {
        std::cerr << "Error: compatibility requires a base schema and a new schema.\n";
        return;
    }

    std::cout << std::format("Comparing: {} <-> {}\n", args[0], args[1]);
    std::cout << "[MOCK] Compatibility check: PASS\n";
}

}
