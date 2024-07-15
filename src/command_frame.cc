#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>   // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>  // std::cout
#include <sstream>   // std::ostringstream

#include "command.h"
#include "utils.h"

static auto enum_to_string(const sourcemeta::jsontoolkit::ReferenceEntryType type) -> std::string {
  switch (type) {
    case sourcemeta::jsontoolkit::ReferenceEntryType::Resource:
      return "resource";
    case sourcemeta::jsontoolkit::ReferenceEntryType::Anchor:
      return "anchor";
    case sourcemeta::jsontoolkit::ReferenceEntryType::Pointer:
      return "pointer";
    default:
      return "unknown";
  }
}

auto intelligence::jsonschema::cli::frame(const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"json", "j"})};
  CLI_ENSURE(!options.at("").empty(), "You must pass a JSON Schema as input")
  const sourcemeta::jsontoolkit::JSON schema{
      sourcemeta::jsontoolkit::from_file(options.at("").front())};

  sourcemeta::jsontoolkit::ReferenceFrame frame;
  sourcemeta::jsontoolkit::ReferenceMap references;
  sourcemeta::jsontoolkit::frame(schema, frame, references,
                                 sourcemeta::jsontoolkit::default_schema_walker, resolver(options))
      .wait();

  const auto output_json = options.contains("json") || options.contains("j");
  if (output_json) {
    auto output_json_object = sourcemeta::jsontoolkit::JSON::make_object();
    auto frame_json = sourcemeta::jsontoolkit::JSON::make_object();
    auto references_json = sourcemeta::jsontoolkit::JSON::make_object();

    for (const auto &[key, entry] : frame) {
      auto frame_entry = sourcemeta::jsontoolkit::JSON::make_object();
      frame_entry.assign("root", sourcemeta::jsontoolkit::JSON{entry.root.value_or(nullptr)});
      std::ostringstream pointer_stream;
      sourcemeta::jsontoolkit::stringify(entry.pointer, pointer_stream);
      frame_entry.assign("pointer", sourcemeta::jsontoolkit::JSON{pointer_stream.str()});
      frame_entry.assign("base", sourcemeta::jsontoolkit::JSON{entry.base});
      frame_entry.assign("type", sourcemeta::jsontoolkit::JSON{enum_to_string(entry.type)});
      std::ostringstream reference_stream;
      sourcemeta::jsontoolkit::stringify(entry.relative_pointer, reference_stream);
      frame_entry.assign("relativePointer", sourcemeta::jsontoolkit::JSON{reference_stream.str()});
      frame_entry.assign("dialect", sourcemeta::jsontoolkit::JSON{entry.dialect});
      frame_json.assign(key.second, sourcemeta::jsontoolkit::JSON{frame_entry});
    }
    output_json_object.assign("frames", sourcemeta::jsontoolkit::JSON{frame_json});

    for (const auto &[pointer, entry] : references) {
      auto ref_entry = sourcemeta::jsontoolkit::JSON::make_object();
      ref_entry.assign(
          "type", sourcemeta::jsontoolkit::JSON{
                      pointer.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic ? "Dynamic"
                                                                                       : "Static"});
      ref_entry.assign("destination", sourcemeta::jsontoolkit::JSON{entry.destination});
      if (entry.base.has_value()) {
        ref_entry.assign("base", sourcemeta::jsontoolkit::JSON{entry.base.value()});
      } else {
        ref_entry.assign("base", sourcemeta::jsontoolkit::JSON{nullptr});
      }
      if (entry.fragment.has_value()) {
        ref_entry.assign("fragment", sourcemeta::jsontoolkit::JSON{entry.fragment.value()});
      } else {
        ref_entry.assign("fragment", sourcemeta::jsontoolkit::JSON{nullptr});
      }
      std::ostringstream ref_entry_stream;
      sourcemeta::jsontoolkit::stringify(pointer.second, ref_entry_stream);
      references_json.assign(ref_entry_stream.str(), sourcemeta::jsontoolkit::JSON{ref_entry});
    }
    output_json_object.assign("references", sourcemeta::jsontoolkit::JSON{references_json});

    std::ostringstream print_stream;
    sourcemeta::jsontoolkit::prettify(output_json_object, print_stream);
    std::cout << print_stream.str() << std::endl;
  } else {
    for (const auto &[key, entry] : frame) {
      std::cout << "(LOCATION) URI: " << key.second << "\n";
      std::cout << "    Schema           : " << entry.root.value_or("<ANONYMOUS>") << "\n";
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
      std::cout << "    Type             : "
                << (pointer.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic ? "Dynamic"
                                                                                     : "Static")
                << "\n";
      std::cout << "    Destination      : " << entry.destination << "\n";

      if (entry.base.has_value()) {
        std::cout << "    - (w/o fragment) : " << entry.base.value() << "\n";
      }

      if (entry.fragment.has_value()) {
        std::cout << "    - (fragment)     : " << entry.fragment.value() << "\n";
      }
    }
  }

  return EXIT_SUCCESS;
}
