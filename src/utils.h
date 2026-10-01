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

// An OpenAPI description declares no identifier of its own under the revisions
// we support, so the one it is read under is where it came from
inline auto openapi_default_id(const std::filesystem::path &schema_path,
                               const bool from_stdin) -> std::string {
  if (from_stdin) {
    return std::string{STDIN_OPENAPI_DEFAULT_ID};
  }

  return default_id(schema_path, from_stdin);
}

inline auto openapi_default_id(const InputJSON &entry) -> std::string {
  return openapi_default_id(entry.resolution_base, entry.from_stdin);
}

// What an entry point is resolved against is whatever the caller keys its
// locations by. A schema goes by the identifier it declares at its root, while
// a description has no root schema and goes by the base it was read under
inline auto resolve_entrypoint(const std::string_view base,
                               const std::string_view entrypoint)
    -> std::string {
  if (entrypoint.empty()) {
    return std::string{base};
  }

  if (entrypoint.front() == '/' &&
      (entrypoint.size() < 2 || entrypoint[1] != '/')) {
    sourcemeta::core::URI result{std::string{base}};
    result.fragment(entrypoint);
    return result.recompose();
  }

  if (entrypoint.front() == '#') {
    const std::string pointer_string{entrypoint.substr(1)};
    sourcemeta::core::URI result{std::string{base}};
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

inline auto resolve_entrypoint(const sourcemeta::core::SchemaFrame &frame,
                               const std::string_view entrypoint)
    -> std::string {
  return resolve_entrypoint(frame.root(), entrypoint);
}

constexpr std::string_view TEST_DOCUMENT_DEFAULT_DIALECT{
    "https://json-schema.org/draft/2020-12/schema"};

inline auto looks_like_test_document(const sourcemeta::core::JSON &document)
    -> bool {
  return document.is_object() && !document.defines("$schema") &&
         document.defines("target") && document.at("target").is_string() &&
         document.defines("tests") && document.at("tests").is_array();
}

// Whether a document holds an OpenAPI Description at all, whichever revision it
// declares. The `openapi` field is what the specification identifies one by,
// and one we cannot read is a description all the same, so this is what tells a
// description from a schema rather than what tells a readable one from the rest
inline auto is_openapi_document(const sourcemeta::core::JSON &document)
    -> bool {
  if (!document.is_object()) {
    return false;
  }

  const auto *version{document.try_at("openapi")};
  return version != nullptr && version->is_string();
}

// The revision an OpenAPI description declares, when it is one we cannot read.
// A document that declares the field as anything but a string is no OpenAPI
// description by any reading of the specification, so it goes on being read as
// a schema rather than being turned down here
inline auto unsupported_openapi_version(const sourcemeta::core::JSON &document)
    -> const sourcemeta::core::JSON * {
  if (!is_openapi_document(document)) {
    return nullptr;
  }

  return sourcemeta::core::openapi_version(document).has_value()
             ? nullptr
             : document.try_at("openapi");
}

// A description of a revision we cannot read is no JSON Schema either, so it is
// turned down rather than being taken for one. The caller that knows where the
// document came from hands over its positions, so that the revision we cannot
// read is pointed at rather than merely named
inline auto reject_unsupported_openapi(
    const sourcemeta::core::JSON &document, const std::filesystem::path &path,
    const sourcemeta::core::PointerPositionTracker *positions = nullptr)
    -> void {
  const auto *version{unsupported_openapi_version(document)};
  if (version == nullptr) {
    return;
  }

  if (positions != nullptr) {
    const auto position{positions->get(sourcemeta::core::Pointer{"openapi"})};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<UnsupportedOpenAPIVersionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()), path,
          version->to_string());
    }
  }

  throw sourcemeta::core::FileError<UnsupportedOpenAPIVersionError>(
      path, version->to_string());
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

// A description declares no dialect of its own, so where it was read from is
// what takes the place of the one a schema would be ordered against
inline auto format_openapi(sourcemeta::core::JSON &document,
                           const sourcemeta::core::SchemaResolver &resolver,
                           const std::string_view default_base) -> void {
  const sourcemeta::core::OpenAPIFrame frame{
      document, sourcemeta::core::schema_walker, resolver, default_base};
  sourcemeta::core::openapi_format(document, frame);
}

// OpenAPI Specification 3.2.1, Section 4.1 lets a document name itself with
// `$self`, "which also serves as its base URI", resolved against wherever the
// document was retrieved from. RFC 3986 Section 5.2.2 never resolves a
// reference against a fragment, so one written there is no part of the base
inline auto openapi_self_identity(const sourcemeta::core::JSON &document,
                                  const std::string &retrieval)
    -> std::optional<std::string> {
  if (!document.is_object()) {
    return std::nullopt;
  }

  // 3.2 is where the field gains that meaning. Before it, `$self` is no part
  // of the specification and names nothing, so a document that spells one
  // goes by where it was retrieved from like any other
  if (sourcemeta::core::openapi_version(document) !=
      sourcemeta::core::OpenAPIVersion::OPENAPI_3_2) {
    return std::nullopt;
  }

  const auto *self{document.try_at("$self")};
  if (self == nullptr || !self->is_string()) {
    return std::nullopt;
  }

  try {
    sourcemeta::core::URI uri{self->to_string()};
    uri.resolve_from(sourcemeta::core::URI{retrieval});
    uri.canonicalize();
    return uri.recompose_without_fragment();
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }
}

// Every way of reading an OpenAPI description answers for the same failures,
// whether it is being made whole or merely looked at, so they are settled here
// once and each caller runs its own work through this
template <typename Function>
auto with_openapi_diagnostics(
    const sourcemeta::core::JSON &document, const std::string_view entry_base,
    const std::filesystem::path &display_path,
    const sourcemeta::core::PointerPositionTracker &positions,
    const Function &callback) -> decltype(callback()) {
  // Only a place within the entry document is one our positions describe, as
  // a description may span others that we never read
  const auto entry_self{
      openapi_self_identity(document, std::string{entry_base})};
  const auto describes = [&entry_base,
                          &entry_self](const std::string_view base) -> bool {
    return base == entry_base ||
           (entry_self.has_value() && base == entry_self.value());
  };

  try {
    return callback();
  } catch (const sourcemeta::core::OpenAPIError &error) {
    const auto position{describes(error.base())
                            ? positions.get(error.location())
                            : std::nullopt};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          display_path, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>(
        display_path, error);
  } catch (const sourcemeta::core::OpenAPIResolutionError &error) {
    const auto position{describes(error.base())
                            ? positions.get(error.location())
                            : std::nullopt};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::OpenAPIResolutionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          display_path, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIResolutionError>(
        display_path, error);
  } catch (const sourcemeta::core::OpenAPIReferenceError &error) {
    const auto position{describes(error.base())
                            ? positions.get(error.location())
                            : std::nullopt};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::OpenAPIReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          display_path, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIReferenceError>(
        display_path, error);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        display_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        display_path, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          display_path, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(display_path, error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          display_path, error.identifier(), error.location(), error.what());
    }

    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        display_path, error.identifier(), error.location(), error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(display_path,
                                                                   error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        display_path, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(display_path);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(display_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        display_path, error.what());
  }
}

// Blaze does not reach for what a reference names, so every document a
// description spans has to be here before any of it can be compiled
inline auto openapi_bundle_for_evaluation(
    sourcemeta::core::JSON &document,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::OpenAPIResolver &openapi_resolver,
    const std::string_view default_base,
    const std::filesystem::path &display_path,
    const sourcemeta::core::PointerPositionTracker &positions) -> void {
  with_openapi_diagnostics(
      document, default_base, display_path, positions, [&]() {
        sourcemeta::core::openapi_bundle(
            document, sourcemeta::core::schema_walker, resolver,
            openapi_resolver, {.default_base = std::string{default_base}});
      });
}

// Framing a description is what reaches both what it says of itself and the
// Schema Objects it holds
inline auto openapi_frame_for_evaluation(
    const sourcemeta::core::JSON &document,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::string_view default_base,
    const std::filesystem::path &display_path,
    const sourcemeta::core::PointerPositionTracker &positions)
    -> sourcemeta::core::OpenAPIFrame {
  return with_openapi_diagnostics(
      document, default_base, display_path, positions, [&]() {
        return sourcemeta::core::OpenAPIFrame{
            document, sourcemeta::core::schema_walker, resolver, default_base};
      });
}

// A description holds many schemas and no root one, so which of them to work
// from is the caller's to say. Everything up to that point is the same whoever
// asks, and the frame cannot be handed back, as it is neither copyable nor
// movable, so the caller's work comes here instead
template <typename Function>
auto with_openapi_entrypoint(
    sourcemeta::core::JSON &document, const sourcemeta::core::Options &options,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::OpenAPIResolver &openapi_resolver,
    const std::string &openapi_base, const std::filesystem::path &display_path,
    const sourcemeta::core::PointerPositionTracker &positions,
    const Function &callback)
    -> decltype(callback(std::declval<const sourcemeta::core::SchemaFrame &>(),
                         std::declval<const std::string &>())) {
  // An entry point given as an empty string is one nobody means, as an unset
  // variable reaches us that way, so it counts as not having passed one at all
  if (!options.contains("entrypoint") || options.at("entrypoint").empty() ||
      options.at("entrypoint").front().empty()) {
    throw OptionConflictError{
        "You must pass an entry point using the `--entrypoint/-p` option when "
        "the input is an OpenAPI description"};
  }

  openapi_bundle_for_evaluation(document, resolver, openapi_resolver,
                                openapi_base, display_path, positions);

  const auto frame{openapi_frame_for_evaluation(
      document, resolver, openapi_base, display_path, positions)};

  std::string entrypoint_uri;
  try {
    entrypoint_uri =
        resolve_entrypoint(frame.base(), options.at("entrypoint").front());
  } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidEntryPoint>(display_path, error);
  }

  // Asking first means the miss is reported in the description's own terms,
  // rather than by whatever compiles it next, which only knows about schemas
  if (!frame.schemas().traverse(entrypoint_uri).has_value()) {
    throw sourcemeta::core::FileError<OpenAPIEntryPointError>(
        display_path, OpenAPIEntryPointError{entrypoint_uri});
  }

  return callback(frame.schemas(), entrypoint_uri);
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
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error.identifier(), error.location(), error.what());
    }

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
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error.identifier(), error.location(), error.what());
    }

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
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidRegexError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

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
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error);
    }

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
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          resolution_base, error.identifier(), error.location(), error.what());
    }

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

// The instance locations an evaluation reports are relative to whatever was
// validated, so when that is a subtree of the tracked document, as a Schema
// Object of an OpenAPI description is, the tracker only answers once the
// position of that subtree is put back in front. Mirrors the `instance_base`
// that Blaze's standard output takes for the same reason
template <typename Entries>
inline auto print(const Entries &output,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream,
                  const std::string_view error_label = "error:",
                  const sourcemeta::core::Pointer &base = {}) -> void {
  stream << error_label << " Schema validation failure\n";
  for (const auto &entry : output) {
    stream << "  " << entry.message << "\n";
    stream << "    at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"";

    const auto position{tracker.get(
        base.concat(sourcemeta::core::to_pointer(entry.instance_location)))};
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
