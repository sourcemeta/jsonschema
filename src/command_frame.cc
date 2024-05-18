#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr, std::cout, std::endl

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::frame(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};

  if (options.at("").empty()) {
    std::cerr << "You must pass a JSON Schema as input\n";
    return EXIT_FAILURE;
  }

  const sourcemeta::jsontoolkit::JSON schema{
      sourcemeta::jsontoolkit::from_file(options.at("").front())};

  sourcemeta::jsontoolkit::ReferenceFrame frame;
  sourcemeta::jsontoolkit::ReferenceMap references;
  sourcemeta::jsontoolkit::frame(schema, frame, references,
                                 sourcemeta::jsontoolkit::default_schema_walker,
                                 sourcemeta::jsontoolkit::official_resolver)
      .wait();

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
    std::cout << "\n";
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
