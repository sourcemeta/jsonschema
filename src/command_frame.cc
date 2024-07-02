#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

// Function to pad each line of a string except the first line
std::string padLines(const std::string &input, int padding) {
    std::istringstream inputStream(input);
    std::ostringstream outputStream;
    std::string line;
    std::string pad(padding, ' ');

    // Handle the first line
    if (std::getline(inputStream, line)) {
        outputStream << line << "\n";
    }

    while (std::getline(inputStream, line)) {
        outputStream << pad << line << "\n";
    }

    return outputStream.str();
}

auto intelligence::jsonschema::cli::frame(const std::span<const std::string> &arguments) -> int {
    const auto options{parse_options(arguments, {"json", "j"})};
    CLI_ENSURE(!options.at("").empty(), "You must pass a JSON Schema as input")
    const sourcemeta::jsontoolkit::JSON schema{
        sourcemeta::jsontoolkit::from_file(options.at("").front())
    };

    sourcemeta::jsontoolkit::ReferenceFrame frame;
    sourcemeta::jsontoolkit::ReferenceMap references;
    sourcemeta::jsontoolkit::frame(schema, frame, references,
                                   sourcemeta::jsontoolkit::default_schema_walker,
                                   resolver(options))
        .wait();

    bool outputJson = options.contains("json") || options.contains("-j");

    for (const auto &[key, entry] : frame) {
        std::cout << "(LOCATION) URI: ";
        std::cout << key.second << "\n";
        std::cout << "    Schema           : " << entry.root.value_or("<ANONYMOUS>")
                  << "\n";
        std::cout << "    Pointer          :";
        if (!entry.pointer.empty()) {
            std::cout << " ";
        }

        sourcemeta::jsontoolkit::stringify(entry.pointer, std::cout);
        if (outputJson) {
            std::cout << "\n";
            std::cout << "    JSON             : ";
            std::ostringstream jsonStream;
            sourcemeta::jsontoolkit::prettify(sourcemeta::jsontoolkit::get(schema, entry.pointer), jsonStream);
            std::cout << padLines(jsonStream.str(), 23);
        }
        std::cout << "    Base URI         : " << entry.base << "\n";
        std::cout << "    Relative Pointer :";
        if (!entry.relative_pointer.empty()) {
            std::cout << " ";
        }

        sourcemeta::jsontoolkit::stringify(entry.relative_pointer, std::cout);
        std::cout << "\n";
        std::cout << "    Dialect          : " << entry.dialect << "\n";
    }

    for (const auto &[pointer, entry] : references) {
        std::cout << "(REFERENCE) URI: ";
        sourcemeta::jsontoolkit::stringify(pointer.second, std::cout);
        std::cout << "\n";

        std::cout << "    Type             : ";
        if (pointer.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic) {
            std::cout << "Dynamic";
        } else {
            std::cout << "Static";
        }
        std::cout << "\n";
        std::cout << "    Destination      : " << entry.destination << "\n";

        if (entry.base.has_value()) {
            std::cout << "    - (w/o fragment) : " << entry.base.value() << "\n";
        }

        if (entry.fragment.has_value()) {
            std::cout << "    - (fragment)     : " << entry.fragment.value() << "\n";
        }
    }

    return EXIT_SUCCESS;
}
