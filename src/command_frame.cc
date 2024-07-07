#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

const char* enumToString(sourcemeta::jsontoolkit::ReferenceEntryType type) {
    switch(type) {
        case sourcemeta::jsontoolkit::ReferenceEntryType::Resource: return "Resource";
        case sourcemeta::jsontoolkit::ReferenceEntryType::Anchor: return "Anchor";
        case sourcemeta::jsontoolkit::ReferenceEntryType::Pointer: return "Pointer";
        default: return "Unknown";
    }
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

    bool outputJson = options.contains("json") || options.contains("j");
    if (outputJson) {
        sourcemeta::jsontoolkit::JSON outputJson = sourcemeta::jsontoolkit::JSON::make_object();
        auto frameJson = sourcemeta::jsontoolkit::JSON::make_object();
        auto referencesJson = sourcemeta::jsontoolkit::JSON::make_object();

        for (const auto &[key, entry] : frame) {
            sourcemeta::jsontoolkit::JSON frameEntry = sourcemeta::jsontoolkit::JSON::make_object();
            frameEntry.assign("schema", sourcemeta::jsontoolkit::JSON{entry.root.value_or("<ANONYMOUS>")});
            std::ostringstream pointer_stream;
            sourcemeta::jsontoolkit::stringify(entry.pointer, pointer_stream);
            frameEntry.assign("pointer", sourcemeta::jsontoolkit::JSON{pointer_stream.str()});
            frameEntry.assign("baseURI", sourcemeta::jsontoolkit::JSON{entry.base});
            frameEntry.assign("type", sourcemeta::jsontoolkit::JSON{enumToString(entry.type)});
            std::ostringstream reference_stream;
            sourcemeta::jsontoolkit::stringify(entry.relative_pointer, reference_stream);
            frameEntry.assign("relativePointer", sourcemeta::jsontoolkit::JSON{reference_stream.str()});
            frameEntry.assign("dialect", sourcemeta::jsontoolkit::JSON{entry.dialect});
            frameJson.assign(key.second, sourcemeta::jsontoolkit::JSON{frameEntry});
        }
        outputJson.assign("frames", sourcemeta::jsontoolkit::JSON{frameJson});

        for (const auto &[pointer, entry] : references) {
            auto refEntry = sourcemeta::jsontoolkit::JSON::make_object();
            refEntry.assign("type", sourcemeta::jsontoolkit::JSON{pointer.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic ? "Dynamic" : "Static"});
            refEntry.assign("destination", sourcemeta::jsontoolkit::JSON{entry.destination});
            if (entry.base.has_value()) {
                refEntry.assign("fragmentBaseURI", sourcemeta::jsontoolkit::JSON{entry.base.value()});
            }
            if (entry.fragment.has_value()) {
                refEntry.assign("fragment", sourcemeta::jsontoolkit::JSON{entry.fragment.value()});
            }
            std::ostringstream ref_entry_stream;
            sourcemeta::jsontoolkit::stringify(pointer.second, ref_entry_stream);
            referencesJson.assign(ref_entry_stream.str(), sourcemeta::jsontoolkit::JSON{refEntry});
        }
        outputJson.assign("references", sourcemeta::jsontoolkit::JSON{referencesJson});
        
        std::ostringstream print_stream;
        sourcemeta::jsontoolkit::prettify(outputJson, print_stream);
        std::cout << print_stream.str() << std::endl;
    } else {
        for (const auto &[key, entry] : frame) {
            std::cout << "(LOCATION) URI: " << key.second << "\n";
            std::cout << "    Schema           : " << entry.root.value_or("<ANONYMOUS>") << "\n";
            std::cout << "    Pointer          :";
            if (!entry.pointer.empty()) {
                std::cout << " ";
            }
            std::cout<<"\n";
            sourcemeta::jsontoolkit::stringify(entry.pointer, std::cout);
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
            std::cout << "    Type             : " << (pointer.first == sourcemeta::jsontoolkit::ReferenceType::Dynamic ? "Dynamic" : "Static") << "\n";
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
