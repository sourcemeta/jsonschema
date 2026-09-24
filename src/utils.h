#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>

#include "error.h"
#include "input.h"

#include <algorithm>   // std::max, std::ranges::all_of
#include <cctype>      // std::isdigit
#include <cstddef>     // std::size_t
#include <filesystem>  // std::filesystem::path
#include <memory>      // std::make_shared
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <set>         // std::set
#include <stdexcept>   // std::out_of_range
#include <string>      // std::string, std::stoull
#include <string_view> // std::string_view
#include <thread>      // std::thread
#include <utility>     // std::unreachable

namespace sourcemeta::jsonschema {

inline auto default_id(const std::filesystem::path &schema_path,
                       const bool from_stdin) -> std::string {
  if (from_stdin) {
    return std::string{STDIN_DEFAULT_ID};
  }

  return sourcemeta::core::URI::from_path(
             sourcemeta::core::weakly_canonical(schema_path))
      .recompose();
}

inline auto resolve_relative_uri(const std::string &value,
                                 const std::filesystem::path &base,
                                 const std::set<std::string> &extensions = {})
    -> std::string {
  const sourcemeta::core::URI uri{value};
  if (!uri.is_relative()) {
    return value;
  }

  const auto canonical{
      sourcemeta::core::weakly_canonical(base / uri.to_path())};

  if (!extensions.empty() && !std::filesystem::is_regular_file(canonical)) {
    for (const auto &extension : extensions) {
      if (extension.empty()) {
        continue;
      }

      std::filesystem::path candidate{canonical};
      candidate += extension;
      if (std::filesystem::is_regular_file(candidate)) {
        return sourcemeta::core::URI::from_path(candidate).recompose();
      }
    }
  }

  return sourcemeta::core::URI::from_path(canonical).recompose();
}

inline auto default_id(const InputJSON &entry) -> std::string {
  return default_id(entry.resolution_base, entry.from_stdin);
}

inline auto resolve_entrypoint(const sourcemeta::core::SchemaFrame &frame,
                               const std::string_view entrypoint)
    -> std::string {
  if (entrypoint.empty()) {
    return std::string{frame.root()};
  }

  if (entrypoint.front() == '/' &&
      (entrypoint.size() < 2 || entrypoint[1] != '/')) {
    sourcemeta::core::URI result{frame.root()};
    result.fragment(entrypoint);
    return result.recompose();
  }

  if (entrypoint.front() == '#') {
    const std::string pointer_string{entrypoint.substr(1)};
    sourcemeta::core::URI result{frame.root()};
    result.fragment(pointer_string);
    return result.recompose();
  }

  try {
    const sourcemeta::core::URI uri{entrypoint};
    return std::string{entrypoint};
  } catch (const sourcemeta::core::URIParseError &) {
    throw sourcemeta::blaze::CompilerInvalidEntryPoint{
        entrypoint, "The given entry point is not a valid URI or JSON Pointer"};
  }
}

constexpr std::string_view TEST_DOCUMENT_DEFAULT_DIALECT{
    "https://json-schema.org/draft/2020-12/schema"};

inline auto looks_like_test_document(const sourcemeta::core::JSON &document)
    -> bool {
  return document.is_object() && !document.defines("$schema") &&
         document.defines("target") && document.at("target").is_string() &&
         document.defines("tests") && document.at("tests").is_array();
}

// The revision an OpenAPI description declares, when it is one we cannot read.
// A document that declares the field as anything but a string is no OpenAPI
// description by any reading of the specification, so it goes on being read as
// a schema rather than being turned down here
inline auto unsupported_openapi_version(const sourcemeta::core::JSON &document)
    -> const sourcemeta::core::JSON * {
  if (!document.is_object()) {
    return nullptr;
  }

  const auto *version{document.try_at("openapi")};
  if (version == nullptr || !version->is_string()) {
    return nullptr;
  }

  return sourcemeta::core::openapi_version(document).has_value() ? nullptr
                                                                 : version;
}

inline auto default_dialect(
    const sourcemeta::core::Options &options,
    const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> std::string {
  if (options.contains("default-dialect")) {
    std::string value{options.at("default-dialect").front()};
    try {
      const sourcemeta::core::URI uri{value};
      if (!uri.is_relative()) {
        return value;
      }
    } catch (const sourcemeta::core::URIParseError &) {
      throw InvalidDefaultDialectError{std::move(value)};
    }

    return resolve_relative_uri(value, std::filesystem::current_path(),
                                parse_extensions(options, configuration));
  }

  const auto from_config = configuration.and_then(
      [](const sourcemeta::blaze::Configuration &config)
          -> std::optional<std::string> { return config.default_dialect; });
  if (from_config.has_value()) {
    const sourcemeta::core::URI uri{from_config.value()};
    if (!uri.is_relative()) {
      return from_config.value();
    }

    return resolve_relative_uri(from_config.value(),
                                configuration.value().base_path,
                                parse_extensions(options, configuration));
  }

  return "";
}

inline auto format_schema(sourcemeta::core::JSON &schema,
                          const sourcemeta::core::SchemaResolver &resolver,
                          const std::string_view dialect) -> void {
  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Locations, schema,
      sourcemeta::core::schema_walker, resolver, dialect};
  sourcemeta::core::schema_format(schema, frame);
}

inline auto
write_schema(const sourcemeta::core::JSON &schema, std::ostream &stream,
             const std::optional<std::size_t> indentation,
             const std::optional<sourcemeta::core::YAMLRoundTrip> &roundtrip)
    -> void {
  if (roundtrip.has_value()) {
    sourcemeta::core::stringify_yaml(schema, stream, roundtrip.value(),
                                     indentation);
    return;
  }

  sourcemeta::core::prettify(schema, stream, indentation.value_or(2));
  stream << "\n";
}

inline auto parse_jobs(const sourcemeta::core::Options &options)
    -> std::size_t {
  if (options.contains("jobs")) {
    const std::string value{options.at("jobs").front()};
    if (value.empty() || !std::ranges::all_of(value, [](const char character) {
          return std::isdigit(static_cast<unsigned char>(character));
        })) {
      throw InvalidJobsError{};
    }

    std::size_t result{0};
    try {
      result = std::stoull(value);
    } catch (const std::out_of_range &) {
      throw InvalidJobsError{};
    }

    if (result == 0) {
      throw InvalidJobsError{};
    }

    return result;
  }

  // The standard library is allowed to not know the level of concurrency
  // that the current system supports
  return std::max(static_cast<std::size_t>(std::thread::hardware_concurrency()),
                  static_cast<std::size_t>(1));
}

// The width the user asked for, or nothing when they asked for none, which
// leaves a document that carries a width of its own keeping it
inline auto parse_optional_indentation(const sourcemeta::core::Options &options)
    -> std::optional<std::size_t> {
  if (!options.contains("indentation")) {
    return std::nullopt;
  }

  const std::string value{options.at("indentation").front()};
  if (value.empty() || !std::ranges::all_of(value, [](const char character) {
        return std::isdigit(static_cast<unsigned char>(character));
      })) {
    throw InvalidIndentationError{};
  }

  try {
    return std::stoull(value);
  } catch (const std::out_of_range &) {
    throw InvalidIndentationError{};
  }
}

inline auto parse_indentation(const sourcemeta::core::Options &options)
    -> std::size_t {
  return parse_optional_indentation(options).value_or(2);
}

inline auto format_assertion_tweaks(const sourcemeta::core::Options &options)
    -> std::optional<sourcemeta::blaze::Tweaks> {
  if (options.contains("format-assertion")) {
    return sourcemeta::blaze::Tweaks{.format_assertion = true};
  }

  return std::nullopt;
}

inline auto bundle_references_options()
    -> sourcemeta::core::SchemaBundleOptions {
  sourcemeta::core::SchemaBundleOptions options;
  options.mode = sourcemeta::core::SchemaBundleOptions::Mode::References;
  return options;
}

inline auto
bundle_for_evaluation(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const std::string &dialect, const std::string &default_id,
                      const std::filesystem::path &resolution_base,
                      const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::JSON {
  try {
    return sourcemeta::core::schema_bundle(
        schema, sourcemeta::core::schema_walker, resolver, dialect, default_id,
        bundle_references_options());
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        resolution_base, error.what());
  } catch (const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaReferenceObjectResourceError>(
        resolution_base, error.identifier());
  }
}

inline auto
frame_for_evaluation(const sourcemeta::core::JSON &bundled,
                     const sourcemeta::core::SchemaResolver &resolver,
                     const std::string &dialect, const std::string &default_id,
                     const std::filesystem::path &resolution_base,
                     const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::SchemaFrame {
  try {
    return sourcemeta::core::SchemaFrame{
        sourcemeta::core::SchemaFrame::Mode::References,
        bundled,
        sourcemeta::core::schema_walker,
        resolver,
        dialect,
        default_id};
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        resolution_base, error.what());
  }
}

inline auto compile_for_evaluation(
    const sourcemeta::core::JSON &bundled,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::SchemaFrame &frame,
    const std::string &entrypoint_uri, const sourcemeta::blaze::Mode mode,
    const std::optional<sourcemeta::blaze::Tweaks> &tweaks,
    const std::filesystem::path &resolution_base,
    const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::blaze::Template {
  try {
    return sourcemeta::blaze::compile(
        bundled, sourcemeta::core::schema_walker, resolver,
        sourcemeta::blaze::default_schema_compiler, frame, entrypoint_uri, mode,
        tweaks);
  } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidEntryPoint>(resolution_base, error);
  } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidRegexError>(resolution_base, error);
  } catch (const sourcemeta::blaze::CompilerError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>(
        resolution_base, error);
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaVocabularyError>(
        resolution_base, error.uri(), error.what());
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        resolution_base, error.what());
  }
}

inline auto facet_name(const sourcemeta::blaze::JSONLDFacet facet)
    -> std::string_view {
  switch (facet) {
    case sourcemeta::blaze::JSONLDFacet::Type:
      return "type";
    case sourcemeta::blaze::JSONLDFacet::Predicate:
      return "predicate";
    case sourcemeta::blaze::JSONLDFacet::Datatype:
      return "datatype";
    case sourcemeta::blaze::JSONLDFacet::Language:
      return "language";
    case sourcemeta::blaze::JSONLDFacet::Direction:
      return "direction";
    case sourcemeta::blaze::JSONLDFacet::Graph:
      return "graph";
    case sourcemeta::blaze::JSONLDFacet::JSON:
      return "json";
    case sourcemeta::blaze::JSONLDFacet::Container:
      return "container";
    case sourcemeta::blaze::JSONLDFacet::Self:
      return "self";
    case sourcemeta::blaze::JSONLDFacet::Override:
      return "override";
    case sourcemeta::blaze::JSONLDFacet::ValuePredicate:
      return "value";
    case sourcemeta::blaze::JSONLDFacet::Constants:
      return "constants";
    default:
      std::unreachable();
  }
}

template <typename Entries>
inline auto print(const Entries &output,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream,
                  const std::string_view error_label = "error:") -> void {
  stream << error_label << " Schema validation failure\n";
  for (const auto &entry : output) {
    stream << "  " << entry.message << "\n";
    stream << "    at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"";

    const auto position{
        tracker.get(sourcemeta::core::to_pointer(entry.instance_location))};
    if (position.has_value()) {
      const auto [line, column, end_line, end_column] = position.value();
      stream << " (line " << line << ", column " << column << ")";
    }

    stream << "\n";
    stream << "    at evaluate path \"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\"\n";
  }
}

struct ValidationSummary {
  std::size_t validated{0};
  std::size_t failed{0};
  bool stopped{false};
};

inline auto print_summary(const ValidationSummary &summary,
                          const sourcemeta::core::Options &options,
                          std::ostream &stream) -> void {
  if (summary.failed > 0 || options.contains("verbose") ||
      options.contains("debug")) {
    stream << "\n";
  }

  stream << summary.validated << " validated, "
         << (summary.validated - summary.failed) << " passed, "
         << summary.failed << " failed\n";
}

inline auto
trace_callback(const sourcemeta::core::PointerPositionTracker &tracker,
               std::ostream &stream)
    -> sourcemeta::blaze::TraceOutput::Callback {
  auto first = std::make_shared<bool>(true);
  return [&tracker, &stream,
          first](const sourcemeta::blaze::TraceOutput::Entry &entry) -> void {
    if (entry.evaluate_path.empty()) {
      return;
    }

    // To make it easier to read
    if (*first) {
      *first = false;
    } else {
      stream << "\n";
    }

    switch (entry.type) {
      case sourcemeta::blaze::TraceOutput::EntryType::Push:
        stream << "-> (push) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Pass:
        stream << "<- (pass) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Fail:
        stream << "<- (fail) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Annotation:
        stream << "@- (annotation) ";
        break;
      default:
        std::unreachable();
    }

    stream << "\"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\"";
    stream << " (" << entry.name << ")\n";

    if (!entry.annotation.is_null()) {
      stream << "   value ";

      if (entry.annotation.is_object()) {
        sourcemeta::core::stringify(entry.annotation, stream);
      } else {
        sourcemeta::core::prettify(entry.annotation, stream);
      }

      stream << "\n";
    }

    stream << "   at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"";

    const auto position{
        tracker.get(sourcemeta::core::to_pointer(entry.instance_location))};
    if (position.has_value()) {
      const auto [line, column, end_line, end_column] = position.value();
      stream << " (line " << line << ", column " << column << ")";
    }

    stream << "\n";
    stream << "   at keyword location \"" << entry.keyword_location << "\"\n";

    if (entry.vocabulary.has_value()) {
      stream << "   at vocabulary \"" << entry.vocabulary.value() << "\"\n";
    }
  };
}

} // namespace sourcemeta::jsonschema

#endif
