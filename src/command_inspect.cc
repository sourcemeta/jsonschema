#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/yaml.h>

#include <algorithm> // std::sort
#include <iomanip>   // std::setw
#include <iostream>  // std::cout
#include <map>       // std::map
#include <ostream>   // std::ostream
#include <utility>   // std::pair, std::unreachable

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
#include "utils.h"

auto print_frame(std::ostream &stream,
                 const sourcemeta::blaze::SchemaFrame &frame,
                 const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  if (frame.locations().empty()) {
    return;
  }

  for (auto iterator = frame.locations().cbegin();
       iterator != frame.locations().cend(); iterator++) {
    const auto &location{*iterator};

    switch (location.second.type) {
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

    stream << " URI: " << location.first.second << "\n";

    if (location.first.first ==
        sourcemeta::blaze::SchemaReferenceType::Static) {
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
      const auto [line, column, end_line, end_column] = position.value();
      stream << "    File Position     : " << line << ":" << column << "\n";
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
           << sourcemeta::blaze::to_string(location.second.base_dialect)
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
        sourcemeta::blaze::SchemaReferenceType::Static) {
      stream << "    Type              : Static\n";
    } else {
      stream << "    Type              : Dynamic\n";
    }

    const auto position{
        positions.get(sourcemeta::core::to_pointer(reference.first.second))};
    if (position.has_value()) {
      const auto [line, column, end_line, end_column] = position.value();
      stream << "    File Position     : " << line << ":" << column << "\n";
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

auto print_keywords(std::ostream &stream, const sourcemeta::core::JSON &schema,
                    const sourcemeta::blaze::SchemaResolver &resolver,
                    const std::string &dialect) -> void {
  using Summary = std::map<std::pair<std::string, std::string>, std::uint64_t>;
  Summary summary;

  for (const auto &entry : sourcemeta::blaze::SchemaIterator{
           schema, sourcemeta::blaze::schema_walker, resolver, dialect}) {
    if (!entry.subschema.get().is_object()) {
      continue;
    }

    for (const auto &property : entry.subschema.get().as_object()) {
      const auto &walker_result{
          sourcemeta::blaze::schema_walker(property.first, entry.vocabularies)};

      const std::string vocabulary{
          walker_result.vocabulary.has_value()
              ? std::string{sourcemeta::blaze::to_string(
                    *walker_result.vocabulary)}
              : "none"};

      const std::pair<std::string, std::string> key{vocabulary, property.first};
      summary[key] += 1;
    }
  }

  std::vector<std::pair<std::pair<std::string, std::string>, std::uint64_t>>
      entries(summary.cbegin(), summary.cend());
  std::sort(entries.begin(), entries.end(),
            [](const auto &left, const auto &right) {
              return left.second > right.second ||
                     (left.second == right.second && left.first < right.first);
            });

  for (const auto &entry : entries) {
    stream << std::setw(5) << entry.second << " - " << entry.first.second
           << " (" << entry.first.first << ")\n";
  }
}

auto sourcemeta::jsonschema::inspect(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema inspect path/to/schema.json"};
  }

  if (options.contains("keywords") && options.contains("json")) {
    throw OptionConflictError{
        "The --keywords option cannot be used with --json"};
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

  if (!sourcemeta::blaze::is_schema(schema)) {
    throw NotSchemaError{schema_from_stdin ? stdin_path()
                                           : schema_resolution_base};
  }

  const auto configuration_path{find_configuration(schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References};

  try {
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};

    if (options.contains("keywords")) {
      print_keywords(std::cout, schema, custom_resolver, dialect);
      return;
    }

    const auto identifier{
        sourcemeta::blaze::identify(schema, custom_resolver, dialect)};

    frame.analyse(
        schema, sourcemeta::blaze::schema_walker, custom_resolver, dialect,

        // Only use the file-based URI if the schema has no
        // identifier, as otherwise we make the output unnecessarily
        // hard when it comes to debugging schemas
        !identifier.empty() ? ""
                            : sourcemeta::jsonschema::default_id(
                                  schema_resolution_base, schema_from_stdin));
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
    sourcemeta::core::prettify(frame.to_json(positions), std::cout);
    std::cout << "\n";
  } else {
    print_frame(std::cout, frame, positions);
  }
}
