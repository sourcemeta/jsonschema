#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::frame(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema frame path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const sourcemeta::jsontoolkit::JSON schema{
      sourcemeta::jsontoolkit::from_file(options.at("").front())};

  sourcemeta::jsontoolkit::ReferenceFrame frame;
  sourcemeta::jsontoolkit::ReferenceMap references;
  sourcemeta::jsontoolkit::frame(schema, frame, references,
                                 sourcemeta::jsontoolkit::default_schema_walker,
                                 resolver(options))
      .wait();

  for (const auto &[key, entry] : frame) {
    switch (entry.type) {
      case sourcemeta::jsontoolkit::ReferenceEntryType::Resource:
        std::cout << "(LOCATION)";
        break;
      case sourcemeta::jsontoolkit::ReferenceEntryType::Anchor:
        std::cout << "(ANCHOR)";
        break;
      case sourcemeta::jsontoolkit::ReferenceEntryType::Pointer:
        std::cout << "(POINTER)";
        break;
      default:
        // We should never get here
        assert(false);
        std::cout << "(UNKNOWN)";
        break;
    }

    std::cout << " URI: ";
    std::cout << key.second << "\n";

    std::cout << "    Type             : ";
    if (key.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic) {
      std::cout << "Dynamic";
    } else {
      std::cout << "Static";
    }
    std::cout << "\n";

    std::cout << "    Root             : " << entry.root.value_or("<ANONYMOUS>")
              << "\n";
    std::cout << "    Pointer          :";
    if (!entry.pointer.empty()) {
      std::cout << " ";
    }

    sourcemeta::jsontoolkit::stringify(entry.pointer, std::cout);
    std::cout << "\n";
    std::cout << "    Base             : " << entry.base << "\n";
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
