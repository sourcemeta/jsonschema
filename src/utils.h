#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/uri.h>

#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>

#include "error.h"
#include "input.h"

#include <filesystem>  // std::filesystem::path
#include <memory>      // std::make_shared
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <set>         // std::set
#include <string>      // std::string, std::stoull
#include <string_view> // std::string_view
#include <utility>     // std::unreachable
#include <variant>     // std::visit

namespace sourcemeta::jsonschema {

inline auto default_id(const std::filesystem::path &schema_path)
    -> std::string {
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
  return default_id(entry.resolution_base);
}

inline auto resolve_entrypoint(const sourcemeta::blaze::SchemaFrame &frame,
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

inline auto parse_indentation(const sourcemeta::core::Options &options)
    -> std::size_t {
  if (options.contains("indentation")) {
    return std::stoull(std::string{options.at("indentation").front()});
  }

  return 2;
}

inline auto format_assertion_tweaks(const sourcemeta::core::Options &options)
    -> std::optional<sourcemeta::blaze::Tweaks> {
  if (options.contains("format-assertion")) {
    return sourcemeta::blaze::Tweaks{.format_assertion = true};
  }

  return std::nullopt;
}

inline auto
bundle_for_evaluation(const sourcemeta::core::JSON &schema,
                      const sourcemeta::blaze::SchemaResolver &resolver,
                      const std::string &dialect, const std::string &default_id,
                      const std::filesystem::path &resolution_base,
                      const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::JSON {
  try {
    return sourcemeta::blaze::bundle(
        schema, sourcemeta::blaze::schema_walker, resolver,
        sourcemeta::blaze::BundleMode::References, dialect, default_id);
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        resolution_base, error.what());
  } catch (const sourcemeta::blaze::SchemaReferenceObjectResourceError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaReferenceObjectResourceError>(
        resolution_base, error.identifier());
  }
}

inline auto
frame_for_evaluation(sourcemeta::blaze::SchemaFrame &frame,
                     const sourcemeta::core::JSON &bundled,
                     const sourcemeta::blaze::SchemaResolver &resolver,
                     const std::string &dialect, const std::string &default_id,
                     const std::filesystem::path &resolution_base,
                     const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  try {
    frame.analyse(bundled, sourcemeta::blaze::schema_walker, resolver, dialect,
                  default_id);
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        resolution_base, error.what());
  }
}

inline auto compile_for_evaluation(
    const sourcemeta::core::JSON &bundled,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const sourcemeta::blaze::SchemaFrame &frame,
    const std::string &entrypoint_uri, const sourcemeta::blaze::Mode mode,
    const std::optional<sourcemeta::blaze::Tweaks> &tweaks,
    const std::filesystem::path &resolution_base,
    const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::blaze::Template {
  try {
    return sourcemeta::blaze::compile(
        bundled, sourcemeta::blaze::schema_walker, resolver,
        sourcemeta::blaze::default_schema_compiler, frame, entrypoint_uri, mode,
        tweaks);
  } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidEntryPoint>(resolution_base, error);
  } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidRegexError>(resolution_base, error);
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaReferenceError>(
        resolution_base, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        resolution_base, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(resolution_base);
  } catch (const sourcemeta::blaze::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaVocabularyError>(
        resolution_base, error.uri(), error.what());
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        resolution_base, error.what());
  }
}

template <typename Entries>
inline auto print(const Entries &output,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream) -> void {
  stream << "error: Schema validation failure\n";
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

inline auto
print_annotations(const sourcemeta::blaze::SimpleOutput &output,
                  const sourcemeta::core::Options &options,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream) -> void {
  if (options.contains("verbose")) {
    for (const auto &annotation : output.annotations()) {
      stream << "annotation: ";
      sourcemeta::core::stringify(annotation.value, stream);
      stream << "\n  at instance location \"";
      sourcemeta::core::stringify(annotation.instance_location, stream);
      stream << "\"";

      const auto position{tracker.get(
          sourcemeta::core::to_pointer(annotation.instance_location))};
      if (position.has_value()) {
        const auto [line, column, end_line, end_column] = position.value();
        stream << " (line " << line << ", column " << column << ")";
      }

      stream << "\n  at evaluate path \"";
      sourcemeta::core::stringify(annotation.evaluate_path, stream);
      stream << "\"\n";
    }
  }
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

    if (entry.vocabulary.first) {
      stream << "   at vocabulary \"";
      if (entry.vocabulary.second.has_value()) {
        std::visit([&stream](const auto &vocabulary) { stream << vocabulary; },
                   entry.vocabulary.second.value());
      } else {
        stream << "<unknown>";
      }
      stream << "\"\n";
    }
  };
}

} // namespace sourcemeta::jsonschema

#endif
