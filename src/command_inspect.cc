#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <iostream> // std::cout
#include <ostream>  // std::ostream

#include "command.h"
#include "error.h"
#include "utils.h"

auto print_frame(std::ostream &stream,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  if (frame.locations().empty()) {
    return;
  }

  for (auto iterator = frame.locations().cbegin();
       iterator != frame.locations().cend(); iterator++) {
    const auto &location{*iterator};

    switch (location.second.type) {
      case sourcemeta::core::SchemaFrame::LocationType::Resource:
        stream << "(RESOURCE)";
        break;
      case sourcemeta::core::SchemaFrame::LocationType::Anchor:
        stream << "(ANCHOR)";
        break;
      case sourcemeta::core::SchemaFrame::LocationType::Pointer:
        stream << "(POINTER)";
        break;
      case sourcemeta::core::SchemaFrame::LocationType::Subschema:
        stream << "(SUBSCHEMA)";
        break;
      default:
        assert(false);
    }

    stream << " URI: " << location.first.second << "\n";

    if (location.first.first == sourcemeta::core::SchemaReferenceType::Static) {
      stream << "    Type              : Static\n";
    } else {
      stream << "    Type              : Dynamic\n";
    }

    stream << "    Root              : "
           << location.second.root.value_or("<ANONYMOUS>") << "\n";

    if (location.second.pointer.empty()) {
      stream << "    Pointer           :\n";
    } else {
      stream << "    Pointer           : ";
      sourcemeta::core::stringify(location.second.pointer, stream);
      stream << "\n";
    }

    const auto position{positions.get(location.second.pointer)};
    if (position.has_value()) {
      stream << "    File Position     : " << std::get<0>(position.value())
             << ":" << std::get<1>(position.value()) << "\n";
    } else {
      stream << "    File Position     : <unknown>:<unknown>\n";
    }

    stream << "    Base              : " << location.second.base << "\n";

    if (location.second.relative_pointer.empty()) {
      stream << "    Relative Pointer  :\n";
    } else {
      stream << "    Relative Pointer  : ";
      sourcemeta::core::stringify(location.second.relative_pointer, stream);
      stream << "\n";
    }

    stream << "    Dialect           : " << location.second.dialect << "\n";
    stream << "    Base Dialect      : " << location.second.base_dialect
           << "\n";

    if (location.second.parent.has_value()) {
      if (location.second.parent.value().empty()) {
        stream << "    Parent            :\n";
      } else {
        stream << "    Parent            : ";
        sourcemeta::core::stringify(location.second.parent.value(), stream);
        stream << "\n";
      }
    } else {
      stream << "    Parent            : <NONE>\n";
    }

    const auto &instance_locations{frame.instance_locations(location.second)};
    if (!instance_locations.empty()) {
      for (const auto &instance_location : instance_locations) {
        if (instance_location.empty()) {
          stream << "    Instance Location :\n";
        } else {
          stream << "    Instance Location : ";
          sourcemeta::core::stringify(instance_location, stream);
          stream << "\n";
        }
      }
    }

    if (std::next(iterator) != frame.locations().cend()) {
      stream << "\n";
    }
  }

  for (auto iterator = frame.references().cbegin();
       iterator != frame.references().cend(); iterator++) {
    stream << "\n";
    const auto &reference{*iterator};
    stream << "(REFERENCE) ORIGIN: ";
    sourcemeta::core::stringify(reference.first.second, stream);
    stream << "\n";

    if (reference.first.first ==
        sourcemeta::core::SchemaReferenceType::Static) {
      stream << "    Type              : Static\n";
    } else {
      stream << "    Type              : Dynamic\n";
    }

    const auto position{positions.get(reference.first.second)};
    if (position.has_value()) {
      stream << "    File Position     : " << std::get<0>(position.value())
             << ":" << std::get<1>(position.value()) << "\n";
    } else {
      stream << "    File Position     : <unknown>:<unknown>\n";
    }

    stream << "    Destination       : " << reference.second.destination
           << "\n";
    stream << "    - (w/o fragment)  : "
           << reference.second.base.value_or("<NONE>") << "\n";
    stream << "    - (fragment)      : "
           << reference.second.fragment.value_or("<NONE>") << "\n";
  }
}

auto sourcemeta::jsonschema::cli::inspect(
    const sourcemeta::core::Options &options) -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema inspect path/to/schema.json"};
  }

  const std::filesystem::path schema_path{options.positional().front()};
  sourcemeta::core::PointerPositionTracker positions;
  const sourcemeta::core::JSON schema{
      sourcemeta::core::read_yaml_or_json(schema_path, std::ref(positions))};

  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{read_configuration(options, configuration_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  const auto identifier{sourcemeta::core::identify(
      schema, custom_resolver,
      sourcemeta::core::SchemaIdentificationStrategy::Strict, dialect)};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Instances};

  frame.analyse(
      schema, sourcemeta::core::schema_official_walker, custom_resolver,
      dialect,

      // Only use the file-based URI if the schema has no identifier,
      // as otherwise we make the output unnecessarily hard when it
      // comes to debugging schemas
      identifier.has_value()
          ? std::optional<sourcemeta::core::JSON::String>(std::nullopt)
          : sourcemeta::core::URI::from_path(
                sourcemeta::core::weakly_canonical(schema_path))
                .recompose());

  if (options.contains("json")) {
    sourcemeta::core::prettify(frame.to_json(positions), std::cout);
    std::cout << "\n";
  } else {
    print_frame(std::cout, frame, positions);
  }
}
