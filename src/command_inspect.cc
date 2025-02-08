#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

static auto
enum_to_string(const sourcemeta::core::SchemaFrame::LocationType type)
    -> std::string {
  switch (type) {
    case sourcemeta::core::SchemaFrame::LocationType::Resource:
      return "resource";
    case sourcemeta::core::SchemaFrame::LocationType::Anchor:
      return "anchor";
    case sourcemeta::core::SchemaFrame::LocationType::Pointer:
      return "pointer";
    case sourcemeta::core::SchemaFrame::LocationType::Subschema:
      return "subschema";
    default:
      return "unknown";
  }
}

auto sourcemeta::jsonschema::cli::inspect(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema inspect path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const sourcemeta::core::JSON schema{
      sourcemeta::jsonschema::cli::read_file(options.at("").front())};

  sourcemeta::core::SchemaFrame frame;
  frame.analyse(schema, sourcemeta::core::schema_official_walker,
                resolver(options));

  const auto output_json = options.contains("json") || options.contains("j");
  if (output_json) {
    auto output_json_object = sourcemeta::core::JSON::make_object();
    auto frame_json = sourcemeta::core::JSON::make_object();
    auto references_json = sourcemeta::core::JSON::make_object();

    for (const auto &[key, entry] : frame.locations()) {
      auto frame_entry = sourcemeta::core::JSON::make_object();
      if (entry.root.has_value()) {
        frame_entry.assign("root", sourcemeta::core::JSON{entry.root.value()});
      } else {
        frame_entry.assign("root", sourcemeta::core::JSON{nullptr});
      }
      std::ostringstream pointer_stream;
      sourcemeta::core::stringify(entry.pointer, pointer_stream);
      frame_entry.assign("pointer",
                         sourcemeta::core::JSON{pointer_stream.str()});
      frame_entry.assign("base", sourcemeta::core::JSON{entry.base});
      frame_entry.assign("type",
                         sourcemeta::core::JSON{enum_to_string(entry.type)});
      std::ostringstream reference_stream;
      sourcemeta::core::stringify(entry.relative_pointer, reference_stream);
      frame_entry.assign("relativePointer",
                         sourcemeta::core::JSON{reference_stream.str()});
      frame_entry.assign("dialect", sourcemeta::core::JSON{entry.dialect});
      frame_entry.assign("baseDialect",
                         sourcemeta::core::JSON{entry.base_dialect});
      frame_json.assign(key.second, sourcemeta::core::JSON{frame_entry});
    }
    output_json_object.assign("frames", sourcemeta::core::JSON{frame_json});

    for (const auto &[pointer, entry] : frame.references()) {
      auto ref_entry = sourcemeta::core::JSON::make_object();
      ref_entry.assign(
          "type",
          sourcemeta::core::JSON{
              pointer.first == sourcemeta::core::SchemaReferenceType::Dynamic
                  ? "dynamic"
                  : "static"});
      ref_entry.assign("destination",
                       sourcemeta::core::JSON{entry.destination});
      if (entry.base.has_value()) {
        ref_entry.assign("base", sourcemeta::core::JSON{entry.base.value()});
      } else {
        ref_entry.assign("base", sourcemeta::core::JSON{nullptr});
      }
      if (entry.fragment.has_value()) {
        ref_entry.assign("fragment",
                         sourcemeta::core::JSON{entry.fragment.value()});
      } else {
        ref_entry.assign("fragment", sourcemeta::core::JSON{nullptr});
      }
      std::ostringstream ref_entry_stream;
      sourcemeta::core::stringify(pointer.second, ref_entry_stream);
      references_json.assign(ref_entry_stream.str(),
                             sourcemeta::core::JSON{ref_entry});
    }
    output_json_object.assign("references",
                              sourcemeta::core::JSON{references_json});

    std::ostringstream print_stream;
    sourcemeta::core::prettify(output_json_object, print_stream);
    std::cout << print_stream.str() << std::endl;
  } else {
    for (const auto &[key, entry] : frame.locations()) {
      switch (entry.type) {
        case sourcemeta::core::SchemaFrame::LocationType::Resource:
          std::cout << "(LOCATION)";
          break;
        case sourcemeta::core::SchemaFrame::LocationType::Anchor:
          std::cout << "(ANCHOR)";
          break;
        case sourcemeta::core::SchemaFrame::LocationType::Pointer:
          std::cout << "(POINTER)";
          break;
        case sourcemeta::core::SchemaFrame::LocationType::Subschema:
          std::cout << "(SUBSCHEMA)";
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
      if (key.first == sourcemeta::core::SchemaReferenceType::Dynamic) {
        std::cout << "Dynamic";
      } else {
        std::cout << "Static";
      }
      std::cout << "\n";

      std::cout << "    Schema           : "
                << entry.root.value_or("<ANONYMOUS>") << "\n";
      std::cout << "    Pointer          :";
      if (!entry.pointer.empty()) {
        std::cout << " ";
      }
      sourcemeta::core::stringify(entry.pointer, std::cout);
      std::cout << "\n";
      std::cout << "    Base URI         : " << entry.base << "\n";
      std::cout << "    Relative Pointer :";
      if (!entry.relative_pointer.empty()) {
        std::cout << " ";
      }
      sourcemeta::core::stringify(entry.relative_pointer, std::cout);
      std::cout << "\n";
      std::cout << "    Dialect          : " << entry.dialect << "\n";
      std::cout << "    Base Dialect     : " << entry.base_dialect << "\n";
    }

    for (const auto &[pointer, entry] : frame.references()) {
      std::cout << "(REFERENCE) URI: ";
      sourcemeta::core::stringify(pointer.second, std::cout);
      std::cout << "\n";
      std::cout << "    Type             : "
                << (pointer.first ==
                            sourcemeta::core::SchemaReferenceType::Dynamic
                        ? "Dynamic"
                        : "Static")
                << "\n";
      std::cout << "    Destination      : " << entry.destination << "\n";

      if (entry.base.has_value()) {
        std::cout << "    - (w/o fragment) : " << entry.base.value() << "\n";
      }

      if (entry.fragment.has_value()) {
        std::cout << "    - (fragment)     : " << entry.fragment.value()
                  << "\n";
      }
    }
  }

  return EXIT_SUCCESS;
}
