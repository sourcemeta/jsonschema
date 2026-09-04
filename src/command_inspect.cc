#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/yaml.h>

#include <iostream> // std::cout
#include <optional> // std::optional
#include <ostream>  // std::ostream
#include <utility>  // std::unreachable

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
#include "utils.h"

auto print_location(std::ostream &stream,
                    const sourcemeta::blaze::SchemaFrame &frame,
                    const sourcemeta::blaze::SchemaResolver &resolver,
                    const sourcemeta::core::PointerPositionTracker &positions,
                    const sourcemeta::blaze::SchemaReferenceType type,
                    const std::string_view uri,
                    const sourcemeta::blaze::SchemaFrame::Location &location)
    -> void {
  switch (location.type) {
    case sourcemeta::blaze::SchemaFrame::LocationType::Resource:
      stream << "(RESOURCE)";
      break;
    case sourcemeta::blaze::SchemaFrame::LocationType::Anchor:
      stream << "(ANCHOR)";
      break;
    case sourcemeta::blaze::SchemaFrame::LocationType::Pointer:
      stream << "(POINTER)";
      break;
    case sourcemeta::blaze::SchemaFrame::LocationType::Subschema:
      stream << "(SUBSCHEMA)";
      break;
    default:
      std::unreachable();
  }

  stream << " URI: " << uri << "\n";

  if (type == sourcemeta::blaze::SchemaReferenceType::Static) {
    stream << "    Type              : Static\n";
  } else {
    stream << "    Type              : Dynamic\n";
  }

  stream << "    Root              : "
         << (frame.root().empty() ? "<ANONYMOUS>" : frame.root()) << "\n";

  if (location.pointer.empty()) {
    stream << "    Pointer           :\n";
  } else {
    stream << "    Pointer           : ";
    sourcemeta::core::stringify(location.pointer, stream);
    stream << "\n";
  }

  const auto position{
      positions.get(sourcemeta::core::to_pointer(location.pointer))};
  if (position.has_value()) {
    const auto [line, column, end_line, end_column] = position.value();
    stream << "    File Position     : " << line << ":" << column << "\n";
  } else {
    stream << "    File Position     : <unknown>:<unknown>\n";
  }

  stream << "    Base              : " << location.base << "\n";

  const auto relative_pointer{
      location.pointer.slice(location.relative_pointer)};
  if (relative_pointer.empty()) {
    stream << "    Relative Pointer  :\n";
  } else {
    stream << "    Relative Pointer  : ";
    sourcemeta::core::stringify(relative_pointer, stream);
    stream << "\n";
  }

  stream << "    Dialect           : " << location.dialect << "\n";
  stream << "    Base Dialect      : " << location.base_dialect << "\n";

  if (location.parent.has_value()) {
    if (location.parent.value().empty()) {
      stream << "    Parent            :\n";
    } else {
      stream << "    Parent            : ";
      sourcemeta::core::stringify(location.parent.value(), stream);
      stream << "\n";
    }
  } else {
    stream << "    Parent            : <NONE>\n";
  }

  if (location.property_name) {
    stream << "    Property Name     : yes\n";
  } else {
    stream << "    Property Name     : no\n";
  }

  if (location.orphan) {
    stream << "    Orphan            : yes\n";
  } else {
    stream << "    Orphan            : no\n";
  }

  if (frame.has_references_to(location.pointer)) {
    stream << "    Referenced        : yes\n";
  } else {
    stream << "    Referenced        : no\n";
  }

  if (frame.has_references_through(location.pointer)) {
    stream << "    Referenced Within : yes\n";
  } else {
    stream << "    Referenced Within : no\n";
  }

  stream << "    Vocabularies      :\n";
  frame.vocabularies(location, resolver)
      .for_each(
          [&stream](
              const sourcemeta::blaze::SchemaVocabularies::URI &vocabulary,
              const bool required) -> void {
            stream << "      " << vocabulary << " ("
                   << (required ? "required" : "optional") << ")\n";
          });
}

auto print_reference(std::ostream &stream,
                     const sourcemeta::core::PointerPositionTracker &positions,
                     const sourcemeta::blaze::SchemaReferenceType type,
                     const sourcemeta::core::WeakPointer &origin,
                     const sourcemeta::blaze::SchemaFrame::Reference &reference)
    -> void {
  stream << "(REFERENCE) ORIGIN: ";
  sourcemeta::core::stringify(origin, stream);
  stream << "\n";

  if (type == sourcemeta::blaze::SchemaReferenceType::Static) {
    stream << "    Type              : Static\n";
  } else {
    stream << "    Type              : Dynamic\n";
  }

  const auto position{positions.get(sourcemeta::core::to_pointer(origin))};
  if (position.has_value()) {
    const auto [line, column, end_line, end_column] = position.value();
    stream << "    File Position     : " << line << ":" << column << "\n";
  } else {
    stream << "    File Position     : <unknown>:<unknown>\n";
  }

  stream << "    Original          : " << reference.original << "\n";
  stream << "    Destination       : " << reference.destination << "\n";
  stream << "    - (w/o fragment)  : "
         << (reference.base.empty() ? "<NONE>" : reference.base) << "\n";
  stream << "    - (fragment)      : " << reference.fragment.value_or("<NONE>")
         << "\n";
}

auto print_frame(std::ostream &stream,
                 const sourcemeta::blaze::SchemaFrame &frame,
                 const sourcemeta::blaze::SchemaResolver &resolver,
                 const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  if (frame.location_count() == 0) {
    return;
  }

  bool first{true};
  frame.for_each_location(
      [&stream, &frame, &resolver, &positions, &first](
          const sourcemeta::blaze::SchemaReferenceType type,
          const std::string_view uri,
          const sourcemeta::blaze::SchemaFrame::Location &location) -> void {
        if (first) {
          first = false;
        } else {
          stream << "\n";
        }

        print_location(stream, frame, resolver, positions, type, uri, location);
      });

  frame.for_each_reference(
      [&stream, &positions](
          const sourcemeta::blaze::SchemaReferenceType type,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::blaze::SchemaFrame::Reference &reference) -> void {
        stream << "\n";
        print_reference(stream, positions, type, origin, reference);
      });
}

auto sourcemeta::jsonschema::inspect(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().empty()) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema inspect path/to/schema.json"};
  }

  validate_http_headers(options);

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  const auto schema_config_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};
  const auto schema_resolution_base{schema_from_stdin ? stdin_path()
                                                      : schema_path};

  sourcemeta::core::PointerPositionTracker positions;
  auto property_storage = std::make_shared<std::deque<std::string>>();
  const sourcemeta::core::JSON schema{[&]() {
    if (schema_from_stdin) {
      auto parsed{read_from_stdin()};
      positions = std::move(parsed.positions);
      property_storage = std::move(parsed.property_storage);
      return std::move(parsed.document);
    }
    sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_yaml_or_json(schema_path, document, callback);
    return document;
  }()};

  if (!schema.is_object() && !schema.is_boolean()) {
    throw NotSchemaError{schema_from_stdin ? stdin_path()
                                           : schema_resolution_base};
  }

  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  std::optional<sourcemeta::blaze::SchemaFrame> frame;

  try {
    frame.emplace(sourcemeta::blaze::SchemaFrame::Mode::Pointers, schema,
                  sourcemeta::blaze::schema_walker, custom_resolver, dialect,
                  sourcemeta::jsonschema::default_id(schema_resolution_base,
                                                     schema_from_stdin),
                  sourcemeta::blaze::SchemaFrame::IdentifierMode::Fallback);
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(schema_resolution_base,
                                                       error);
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(schema_resolution_base);
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        schema_resolution_base, error.what());
  }

  if (options.contains("json")) {
    sourcemeta::core::prettify(
        frame.value().to_json(custom_resolver, positions), std::cout);
    std::cout << "\n";
  } else {
    print_frame(std::cout, frame.value(), custom_resolver, positions);
  }
}
