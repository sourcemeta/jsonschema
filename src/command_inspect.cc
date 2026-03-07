#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <iostream> // std::cout
#include <ostream>  // std::ostream

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
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
           << (frame.root().empty() ? "<ANONYMOUS>" : frame.root()) << "\n";

    if (location.second.pointer.empty()) {
      stream << "    Pointer           :\n";
    } else {
      stream << "    Pointer           : ";
      sourcemeta::core::stringify(location.second.pointer, stream);
      stream << "\n";
    }

    const auto position{
        positions.get(sourcemeta::core::to_pointer(location.second.pointer))};
    if (position.has_value()) {
      stream << "    File Position     : " << std::get<0>(position.value())
             << ":" << std::get<1>(position.value()) << "\n";
    } else {
      stream << "    File Position     : <unknown>:<unknown>\n";
    }

    stream << "    Base              : " << location.second.base << "\n";

    const auto relative_pointer{
        location.second.pointer.slice(location.second.relative_pointer)};
    if (relative_pointer.empty()) {
      stream << "    Relative Pointer  :\n";
    } else {
      stream << "    Relative Pointer  : ";
      sourcemeta::core::stringify(relative_pointer, stream);
      stream << "\n";
    }

    stream << "    Dialect           : " << location.second.dialect << "\n";
    stream << "    Base Dialect      : "
           << sourcemeta::core::to_string(location.second.base_dialect) << "\n";

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

    if (location.second.property_name) {
      stream << "    Property Name     : yes\n";
    } else {
      stream << "    Property Name     : no\n";
    }

    if (location.second.orphan) {
      stream << "    Orphan            : yes\n";
    } else {
      stream << "    Orphan            : no\n";
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

    const auto position{
        positions.get(sourcemeta::core::to_pointer(reference.first.second))};
    if (position.has_value()) {
      stream << "    File Position     : " << std::get<0>(position.value())
             << ":" << std::get<1>(position.value()) << "\n";
    } else {
      stream << "    File Position     : <unknown>:<unknown>\n";
    }

    stream << "    Destination       : " << reference.second.destination
           << "\n";
    stream << "    - (w/o fragment)  : "
           << (reference.second.base.empty() ? "<NONE>" : reference.second.base)
           << "\n";
    stream << "    - (fragment)      : "
           << reference.second.fragment.value_or("<NONE>") << "\n";
  }
}

auto sourcemeta::jsonschema::inspect(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema inspect path/to/schema.json"};
  }

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory",
        schema_path, std::make_error_code(std::errc::is_a_directory)};
  }

  const auto schema_resolution_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};

  sourcemeta::core::PointerPositionTracker positions;
  const sourcemeta::core::JSON schema{[&]() {
    if (schema_from_stdin) {
      auto parsed{read_from_stdin()};
      positions = std::move(parsed.positions);
      return std::move(parsed.document);
    }
    return sourcemeta::core::read_yaml_or_json(schema_path,
                                               std::ref(positions));
  }()};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_from_stdin ? stdin_error_path()
                                           : schema_resolution_base};
  }

  const auto configuration_path{find_configuration(schema_resolution_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_resolution_base)};
  const auto dialect{default_dialect(options, configuration)};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};

  try {
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};
    const auto identifier{
        sourcemeta::core::identify(schema, custom_resolver, dialect)};

    frame.analyse(
        schema, sourcemeta::core::schema_walker, custom_resolver, dialect,

        // Only use the file-based URI if the schema has no
        // identifier, as otherwise we make the output unnecessarily
        // hard when it comes to debugging schemas
        !identifier.empty()
            ? ""
            : sourcemeta::jsonschema::default_id(schema_resolution_base));
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw FileError<sourcemeta::core::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw FileError<sourcemeta::core::SchemaFrameError>(schema_resolution_base,
                                                        error);
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_resolution_base,
                                                   error.what());
  }

  if (options.contains("json")) {
    sourcemeta::core::prettify(frame.to_json(positions), std::cout);
    std::cout << "\n";
  } else {
    print_frame(std::cout, frame, positions);
  }
}
