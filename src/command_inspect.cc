#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>
#include <sourcemeta/core/yaml.h>

#include <cctype>   // std::toupper
#include <iostream> // std::cout
#include <optional> // std::optional
#include <ostream>  // std::ostream
#include <utility>  // std::unreachable
#include <vector>   // std::vector

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
#include "utils.h"

auto print_location(std::ostream &stream,
                    const sourcemeta::core::SchemaFrame &frame,
                    const sourcemeta::core::SchemaResolver &resolver,
                    const sourcemeta::core::PointerPositionTracker &positions,
                    const sourcemeta::core::SchemaReferenceType type,
                    const std::string_view uri,
                    const sourcemeta::core::SchemaFrame::Location &location)
    -> void {
  switch (location.type) {
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
      std::unreachable();
  }

  stream << " URI: " << uri << "\n";

  if (type == sourcemeta::core::SchemaReferenceType::Static) {
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
          [&stream](const sourcemeta::core::SchemaVocabularies::URI &vocabulary,
                    const bool required) -> void {
            stream << "      " << vocabulary << " ("
                   << (required ? "required" : "optional") << ")\n";
          });
}

auto print_reference(std::ostream &stream,
                     const sourcemeta::core::PointerPositionTracker &positions,
                     const sourcemeta::core::SchemaReferenceType type,
                     const sourcemeta::core::WeakPointer &origin,
                     const sourcemeta::core::SchemaFrame::Reference &reference)
    -> void {
  stream << "(REFERENCE) ORIGIN: ";
  sourcemeta::core::stringify(origin, stream);
  stream << "\n";

  if (type == sourcemeta::core::SchemaReferenceType::Static) {
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

// The frame names every kind it holds, so the label is that name rather than
// one of our own, upcased the way the schema kinds above are
auto print_openapi_kind(std::ostream &stream, const std::string_view name)
    -> void {
  stream << "(";
  for (const auto character : name) {
    stream << static_cast<char>(
        std::toupper(static_cast<unsigned char>(character)));
  }
  stream << ")";
}

auto print_openapi_position(
    std::ostream &stream,
    const sourcemeta::core::PointerPositionTracker &positions,
    const sourcemeta::core::Pointer &pointer) -> void {
  const auto position{positions.get(pointer)};
  if (position.has_value()) {
    const auto [line, column, end_line, end_column] = position.value();
    stream << "    File Position     : " << line << ":" << column << "\n";
  } else {
    stream << "    File Position     : <unknown>:<unknown>\n";
  }
}

auto print_openapi_pointer(std::ostream &stream, const std::string_view label,
                           const sourcemeta::core::Pointer &pointer) -> void {
  if (pointer.empty()) {
    stream << "    " << label << ":\n";
  } else {
    stream << "    " << label << ": ";
    sourcemeta::core::stringify(pointer, stream);
    stream << "\n";
  }
}

auto print_openapi_uris(std::ostream &stream, const std::string_view label,
                        const std::vector<sourcemeta::core::JSON::String> &uris)
    -> void {
  if (uris.empty()) {
    stream << "    " << label << ": <NONE>\n";
    return;
  }

  stream << "    " << label << ":\n";
  for (const auto &uri : uris) {
    stream << "      " << uri << "\n";
  }
}

// The specification lets an operation name a tag that nothing declares, which
// the frame keeps a slot for rather than dropping
auto print_openapi_tags(
    std::ostream &stream,
    const std::vector<std::optional<sourcemeta::core::JSON::String>> &tags)
    -> void {
  if (tags.empty()) {
    stream << "    Tags              : <NONE>\n";
    return;
  }

  stream << "    Tags              :\n";
  for (const auto &tag : tags) {
    stream << "      " << tag.value_or("<NONE>") << "\n";
  }
}

auto print_openapi_info(std::ostream &stream,
                        const sourcemeta::core::OpenAPIInfo &info) -> void {
  stream << "    Info              :\n";
  stream << "      Title           : " << info.title << "\n";
  stream << "      Version         : " << info.version << "\n";
  stream << "      Summary         : " << info.summary.value_or("<NONE>")
         << "\n";
  stream << "      Description     : " << info.description.value_or("<NONE>")
         << "\n";
  stream << "      Terms of Service: "
         << info.terms_of_service.value_or("<NONE>") << "\n";

  if (info.contact.has_value()) {
    stream << "      Contact         :\n";
    stream << "        Name          : "
           << info.contact.value().name.value_or("<NONE>") << "\n";
    stream << "        URL           : "
           << info.contact.value().url.value_or("<NONE>") << "\n";
    stream << "        Email         : "
           << info.contact.value().email.value_or("<NONE>") << "\n";
  } else {
    stream << "      Contact         : <NONE>\n";
  }

  if (info.license.has_value()) {
    stream << "      License         :\n";
    stream << "        Name          : " << info.license.value().name << "\n";
    stream << "        Identifier    : "
           << info.license.value().identifier.value_or("<NONE>") << "\n";
    stream << "        URL           : "
           << info.license.value().url.value_or("<NONE>") << "\n";
  } else {
    stream << "      License         : <NONE>\n";
  }
}

auto print_openapi_location(
    std::ostream &stream, const sourcemeta::core::OpenAPIFrame &frame,
    const sourcemeta::core::PointerPositionTracker &positions,
    const std::string_view uri,
    const sourcemeta::core::OpenAPIFrame::Location &location) -> void {
  print_openapi_kind(stream,
                     sourcemeta::core::openapi_kind_name(location.type));
  stream << " URI: " << uri << "\n";

  print_openapi_pointer(stream, "Pointer           ", location.pointer);
  print_openapi_position(stream, positions, location.pointer);

  if (location.parent.has_value()) {
    stream << "    Parent            : " << location.parent.value() << "\n";
  } else {
    stream << "    Parent            : <NONE>\n";
  }

  // The root of a document and a Schema Object carry one, nothing else does
  if (!location.dialect.empty()) {
    stream << "    Dialect           : " << location.dialect << "\n";
  }

  // A Schema Object alone carries one, where an empty base is as much an
  // answer as any other, so this goes by the kind rather than by the value
  if (location.type == sourcemeta::core::OpenAPIFrame::ObjectKind::Schema) {
    stream << "    Base              : " << location.base << "\n";
  }

  // Only an Object that declares a `$ref` carries these, which is a Reference
  // Object or a Path Item Object standing in for another
  const auto &references{frame.references()};
  const auto reference{references.find(uri)};
  if (reference != references.cend()) {
    stream << "    Original          : " << reference->second.original << "\n";
    stream << "    Destination       : " << reference->second.destination
           << "\n";
    stream << "    Dangling          : "
           << (reference->second.dangling ? "yes" : "no") << "\n";
  }

  // What the description as a whole says has nowhere else to sit, as the root
  // is the one position that stands for all of it
  if (location.pointer.empty()) {
    stream << "    Version           : "
           << sourcemeta::core::openapi_version_name(frame.version()) << "\n";
    stream << "    Base              : " << frame.base() << "\n";
    stream << "    Standalone        : " << (frame.standalone() ? "yes" : "no")
           << "\n";
    print_openapi_info(stream, frame.info());
  }
}

auto print_openapi_operation(
    std::ostream &stream,
    const sourcemeta::core::OpenAPIFrame::Operation &operation) -> void {
  print_openapi_kind(
      stream, sourcemeta::core::openapi_operation_kind_name(operation.kind));
  stream << " URI: " << operation.origin << "\n";

  stream << "    Path              : " << operation.path << "\n";
  stream << "    Method            : " << operation.method << "\n";
  stream << "    Endpoint          : " << operation.endpoint << "\n";
  stream << "    Parent            : " << operation.parent.value_or("<NONE>")
         << "\n";
  print_openapi_tags(stream, operation.tags);
  print_openapi_uris(stream, "Servers           ", operation.servers);
  print_openapi_uris(stream, "Security          ", operation.security);
  print_openapi_uris(stream, "Parameters        ", operation.parameters);
}

auto print_openapi_discriminator(
    std::ostream &stream, const sourcemeta::core::OpenAPIFrame &frame,
    const sourcemeta::core::PointerPositionTracker &positions,
    const sourcemeta::core::OpenAPIFrame::Discriminator &discriminator)
    -> void {
  stream << "(DISCRIMINATOR) URI: " << frame.uri(discriminator.origin) << "\n";
  print_openapi_pointer(stream, "Pointer           ", discriminator.origin);
  print_openapi_position(stream, positions, discriminator.origin);
  stream << "    Destination       : " << discriminator.destination << "\n";
  stream << "    Scope             : " << discriminator.scope << "\n";
  stream << "    Dangling          : "
         << (discriminator.dangling ? "yes" : "no") << "\n";
}

auto print_openapi_security_reference(
    std::ostream &stream,
    const sourcemeta::core::PointerPositionTracker &positions,
    const std::string_view uri,
    const sourcemeta::core::OpenAPIFrame::Reference &reference) -> void {
  stream << "(SECURITY-REFERENCE) URI: " << uri << "\n";
  print_openapi_pointer(stream, "Pointer           ", reference.origin);
  print_openapi_position(stream, positions, reference.origin);
  stream << "    Original          : " << reference.original << "\n";
  stream << "    Destination       : " << reference.destination << "\n";
  stream << "    Dangling          : " << (reference.dangling ? "yes" : "no")
         << "\n";
}

auto print_frame(std::ostream &stream,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::SchemaResolver &resolver,
                 const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  if (frame.location_count() == 0) {
    return;
  }

  bool first{true};
  frame.for_each_location(
      [&stream, &frame, &resolver, &positions, &first](
          const sourcemeta::core::SchemaReferenceType type,
          const std::string_view uri,
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        if (first) {
          first = false;
        } else {
          stream << "\n";
        }

        print_location(stream, frame, resolver, positions, type, uri, location);
      });

  frame.for_each_reference(
      [&stream, &positions](
          const sourcemeta::core::SchemaReferenceType type,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> void {
        stream << "\n";
        print_reference(stream, positions, type, origin, reference);
      });
}

// The schemas go last rather than where the frame exports them, as they are
// the bulk of what comes out and everything the description itself says would
// sit behind them otherwise
auto print_openapi_frame(
    std::ostream &stream, const sourcemeta::core::OpenAPIFrame &frame,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::PointerPositionTracker &positions) -> void {
  bool first{true};
  for (const auto &[uri, location] : frame.locations()) {
    if (first) {
      first = false;
    } else {
      stream << "\n";
    }

    print_openapi_location(stream, frame, positions, uri, location);
  }

  for (const auto &operation : frame.operations()) {
    stream << "\n";
    print_openapi_operation(stream, operation);
  }

  for (const auto &discriminator : frame.discriminators()) {
    stream << "\n";
    print_openapi_discriminator(stream, frame, positions, discriminator);
  }

  for (const auto &[uri, reference] : frame.security_references()) {
    stream << "\n";
    print_openapi_security_reference(stream, positions, uri, reference);
  }

  if (frame.schemas().location_count() > 0) {
    stream << "\n";
    print_frame(stream, frame.schemas(), resolver, positions);
  }
}

// A description carries its Schema Objects with it, so framing it is what
// reaches both halves of what there is to report on
auto inspect_openapi(const sourcemeta::core::Options &options,
                     const sourcemeta::core::JSON &document,
                     const sourcemeta::core::SchemaResolver &resolver,
                     const sourcemeta::core::PointerPositionTracker &positions,
                     const std::filesystem::path &display_path,
                     const std::string &default_base) -> void {
  const auto frame{sourcemeta::jsonschema::openapi_frame_for_evaluation(
      document, resolver, default_base, display_path, positions)};

  if (options.contains("json")) {
    sourcemeta::core::prettify(frame.to_json(positions), std::cout);
    std::cout << "\n";
  } else {
    print_openapi_frame(std::cout, frame, resolver, positions);
  }
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

  // An OpenAPI description is not a schema, so what it goes by wherever we
  // report on it is an identity of its own rather than the one a schema from
  // the same place would take
  const auto is_openapi{is_openapi_document(schema)};
  const auto schema_resolution_base{
      schema_from_stdin ? (is_openapi ? openapi_stdin_path() : stdin_path())
                        : schema_path};

  reject_unsupported_openapi(schema, schema_resolution_base, &positions);

  if (!is_openapi && !schema.is_object() && !schema.is_boolean()) {
    throw NotSchemaError{schema_resolution_base};
  }

  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  // A description is framed as what it is, and what it holds within its Schema
  // Objects comes along with it, so the schemas report the same way either way
  if (is_openapi) {
    inspect_openapi(
        options, schema, custom_resolver, positions, schema_resolution_base,
        openapi_default_id(schema_resolution_base, schema_from_stdin));
    return;
  }

  std::optional<sourcemeta::core::SchemaFrame> frame;

  try {
    frame.emplace(sourcemeta::core::SchemaFrame::Mode::Pointers, schema,
                  sourcemeta::core::schema_walker, custom_resolver, dialect,
                  sourcemeta::jsonschema::default_id(schema_resolution_base,
                                                     schema_from_stdin),
                  sourcemeta::core::SchemaFrame::IdentifierMode::Fallback);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(schema_resolution_base,
                                                      error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_resolution_base, error.identifier(), error.location(),
          error.what());
    }

    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        schema_resolution_base, error.identifier(), error.location(),
        error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(schema_resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
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
