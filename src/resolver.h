#ifndef SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_
#define SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include "error.h"
#include "input.h"
#include "logger.h"
#include "utils.h"

#include <algorithm> // std::ranges::find
#include <cassert>   // assert
#include <chrono>    // std::chrono::seconds
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <exception> // std::exception_ptr, std::current_exception, std::rethrow_exception
#include <filesystem>    // std::filesystem
#include <functional>    // std::function, std::ref
#include <iostream>      // std::cerr
#include <map>           // std::map
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <thread>        // std::this_thread::sleep_for
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility> // std::pair, std::piecewise_construct, std::forward_as_tuple, std::move
#include <vector> // std::vector

namespace sourcemeta::jsonschema {

static constexpr std::uint8_t HTTP_MAXIMUM_RETRIES{3};

// A key that is not a valid URI cannot denote a schema identifier, but it must
// not render the rest of the map unusable either, so it stays as the user
// wrote it
static inline auto canonical_resolve_key(const std::string_view key)
    -> std::string {
  try {
    return sourcemeta::core::URI::canonicalize(key);
  } catch (const sourcemeta::core::URIParseError &) {
    return std::string{key};
  }
}

// The `.json` extension is not a URI concern, so this single alternative is
// tried by hand, and always after the identifier itself, so that a map holding
// both spellings is unambiguous
static inline auto resolve_alternative(const std::string &identifier)
    -> std::string {
  return identifier.ends_with(".json")
             ? identifier.substr(0, identifier.size() - 5)
             : identifier + ".json";
}

// Keys are canonicalized once, up front, rather than on every lookup, which
// would make resolution linear in the size of the map for each reference. When
// several keys canonicalize to the same URI, the one the user already wrote in
// canonical form wins, and otherwise the first in lexicographic order, so that
// the winner never depends on hash iteration order
// TODO: Move this to Blaze's configuration parser, alongside where the values
// of the `resolve` object are already canonicalized. Doing it there would let
// a key that is not a valid URI, and a set of keys that collapse into the same
// canonical URI, be reported as a `ConfigurationParseError` pointing at the
// offending property rather than quietly tolerated, and would spare every
// consumer of a configuration from repeating the work
static inline auto canonical_resolve_map(
    const std::unordered_map<std::string, std::string> &resolve_map)
    -> std::unordered_map<std::string, std::string> {
  const std::map<std::string_view, std::string_view> sorted{
      resolve_map.cbegin(), resolve_map.cend()};

  std::unordered_map<std::string, std::string> result;
  result.reserve(sorted.size());
  for (const auto &entry : sorted) {
    auto canonical{canonical_resolve_key(entry.first)};
    const auto is_canonical{canonical == entry.first};
    const auto match{result.find(canonical)};
    if (match == result.cend()) {
      result.emplace(std::move(canonical), entry.second);
    } else if (is_canonical) {
      match->second = entry.second;
    }
  }

  return result;
}

static inline auto find_resolve_match(
    const std::unordered_map<std::string, std::string> &resolve_map,
    const std::string &identifier)
    -> std::unordered_map<std::string, std::string>::const_iterator {
  if (resolve_map.empty()) {
    return resolve_map.cend();
  }

  // Keys are stored canonicalized, so an identifier spelled the way its key is
  // matches without parsing a URI at all
  auto match{resolve_map.find(identifier)};
  if (match != resolve_map.cend()) {
    return match;
  }

  match = resolve_map.find(resolve_alternative(identifier));
  if (match != resolve_map.cend()) {
    return match;
  }

  // Comparing canonical URIs is what lets a key match however the user spelled
  // it. Canonicalization lowercases the scheme and the host, drops an empty
  // fragment and a default port, resolves dot segments, and normalises
  // percent-encoding, all per RFC 3986. An identifier that no URI can express
  // canonicalizes to itself here, and so falls through as unresolved rather
  // than aborting the command
  const auto canonical{canonical_resolve_key(identifier)};
  if (canonical == identifier) {
    return resolve_map.cend();
  }

  match = resolve_map.find(canonical);
  if (match != resolve_map.cend()) {
    return match;
  }

  return resolve_map.find(resolve_alternative(canonical));
}

static inline auto
resolve_map_uri(const std::unordered_map<std::string, std::string> &resolve_map,
                const std::filesystem::path &base_path,
                const std::string &identifier) -> std::optional<std::string> {
  const auto match{find_resolve_match(resolve_map, identifier)};
  if (match == resolve_map.cend()) {
    return std::nullopt;
  }

  return resolve_relative_uri(match->second, base_path);
}

static constexpr std::string_view HTTP_HEADER_EXAMPLE{
    "--header \"Authorization: Bearer ${TOKEN}\""};

static inline auto parse_http_header(const std::string_view input)
    -> std::pair<std::string_view, std::string_view> {
  const auto colon{input.find(':')};
  if (colon == std::string_view::npos) {
    throw PositionalArgumentError{
        "HTTP headers must be in the form `Name: Value`",
        std::string{HTTP_HEADER_EXAMPLE}};
  }

  const auto raw_name{input.substr(0, colon)};
  if (raw_name.empty()) {
    throw PositionalArgumentError{"HTTP header names cannot be empty",
                                  std::string{HTTP_HEADER_EXAMPLE}};
  }

  for (const auto character : raw_name) {
    if (character == ' ' || character == '\t') {
      throw PositionalArgumentError{
          "HTTP header names cannot contain whitespace",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
    if (static_cast<unsigned char>(character) < 0x20 ||
        static_cast<unsigned char>(character) == 0x7F) {
      throw PositionalArgumentError{
          "HTTP header names cannot contain control characters",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
  }

  auto raw_value{input.substr(colon + 1)};
  while (!raw_value.empty() &&
         (raw_value.front() == ' ' || raw_value.front() == '\t')) {
    raw_value.remove_prefix(1);
  }

  for (const auto character : raw_value) {
    if (character == '\r' || character == '\n' || character == '\0') {
      throw PositionalArgumentError{
          "HTTP header values cannot contain control characters",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
  }

  return {raw_name, raw_value};
}

static inline auto
validate_http_headers(const sourcemeta::core::Options &options) -> void {
  if (!options.contains("header")) {
    return;
  }
  for (const auto &raw : options.at("header")) {
    parse_http_header(raw);
  }
}

static inline auto
collect_http_headers(const sourcemeta::core::Options &options)
    -> std::vector<std::pair<std::string_view, std::string_view>> {
  std::vector<std::pair<std::string_view, std::string_view>> headers;
  if (!options.contains("header")) {
    return headers;
  }
  for (const auto &raw : options.at("header")) {
    headers.emplace_back(parse_http_header(raw));
  }
  return headers;
}

static inline auto http_fetch(const std::string &url,
                              const sourcemeta::core::Options &options)
    -> sourcemeta::core::JSON {
  sourcemeta::core::HTTPSystemRequest request{url};
  for (const auto &header : collect_http_headers(options)) {
    request.header(std::string{header.first}, std::string{header.second});
  }

  sourcemeta::core::HTTPResponse response;
  for (std::uint8_t attempt{1}; attempt <= HTTP_MAXIMUM_RETRIES; ++attempt) {
    LOG_VERBOSE(options) << "Resolving over HTTP (attempt "
                         << static_cast<int>(attempt) << "/"
                         << static_cast<int>(HTTP_MAXIMUM_RETRIES)
                         << "): " << url << "\n";
    try {
      response = request.send();
    } catch (const sourcemeta::core::HTTPError &error) {
      if (attempt == HTTP_MAXIMUM_RETRIES) {
        throw;
      }

      LOG_VERBOSE(options) << "Request failed (" << error.what()
                           << "), retrying...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    if (response.status == sourcemeta::core::HTTP_STATUS_OK) {
      break;
    }

    if (attempt < HTTP_MAXIMUM_RETRIES) {
      LOG_VERBOSE(options) << "Request failed with HTTP "
                           << response.status.code << ", retrying...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (response.status != sourcemeta::core::HTTP_STATUS_OK) {
    throw sourcemeta::core::HTTPStatusError{sourcemeta::core::HTTPMethod::GET,
                                            url, response.status};
  }

  const auto content_type{
      sourcemeta::core::http_header_find(response.headers, "content-type")};
  if (content_type.has_value() && sourcemeta::core::http_content_type_matches(
                                      content_type.value(), "text/yaml")) {
    try {
      return sourcemeta::core::parse_yaml(response.body);
    } catch (const sourcemeta::core::YAMLParseError &error) {
      throw sourcemeta::core::YAMLFileParseError{url, error};
    }
  }

  return sourcemeta::core::parse_json(response.body);
}

static inline auto fetch_schema(const sourcemeta::core::Options &options,
                                std::string_view identifier,
                                const bool remote = true,
                                const bool bundle = false)
    -> sourcemeta::core::SchemaResolverResult {
  auto official_result{sourcemeta::core::schema_resolver(identifier)};
  if (official_result.has_value()) {
    return official_result;
  }

  sourcemeta::core::URI uri;
  try {
    uri = sourcemeta::core::URI{identifier};
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }

  if (uri.is_file()) {
    const auto path{uri.to_path()};
    LOG_DEBUG(options) << "Attempting to read file reference from disk: "
                       << path.generic_string() << "\n";
    if (std::filesystem::exists(path)) {
      return sourcemeta::core::read_yaml_or_json(path);
    }

    return std::nullopt;
  }

  if (remote) {
    const auto scheme{uri.scheme()};
    if (!uri.is_urn() && scheme.has_value() &&
        (scheme.value() == "https" || scheme.value() == "http")) {
      std::string fetch_url{identifier};
      if (bundle) {
        // TODO: Use sourcemeta::core::URI to set query parameters once
        // the URI module supports setters for query strings
        if (fetch_url.find('?') != std::string::npos) {
          fetch_url += "&bundle=1";
        } else {
          fetch_url += "?bundle=1";
        }
      }

      return http_fetch(fetch_url, options);
    }
  }

  return std::nullopt;
}

static inline auto
anonymous_base_dialect(const sourcemeta::core::JSON &schema,
                       const sourcemeta::core::SchemaResolver &resolver)
    -> std::optional<sourcemeta::core::SchemaBaseDialect> {
  if (!schema.is_object()) {
    return std::nullopt;
  }

  try {
    const sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::Root, schema,
        sourcemeta::core::schema_walker, resolver};
    if (!frame.root().empty()) {
      return std::nullopt;
    }

    const auto location{frame.root_location()};
    if (!location.has_value()) {
      return std::nullopt;
    }

    return location.value().get().base_dialect;
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    return std::nullopt;
  }
}

enum class IdentifierKeyword : std::uint8_t { Unknown, Modern, Legacy };

// Whether the dialect in force gives a keyword any meaning at all, which is
// what the vocabularies it draws on say rather than what its base dialect
// happens to name
static inline auto
dialect_defines(const sourcemeta::core::SchemaVocabularies &vocabularies,
                const std::string_view keyword) -> bool {
  return sourcemeta::core::schema_walker(keyword, vocabularies).type !=
         sourcemeta::core::SchemaKeywordType::Unknown;
}

// Every official meta-schema identifies itself using the keyword that its own
// dialect relies on, which reveals that keyword without framing anything
static inline auto identifier_keyword(const std::string_view dialect)
    -> IdentifierKeyword {
  const auto metaschema{sourcemeta::core::schema_resolver(dialect)};
  if (!metaschema.has_value()) {
    return IdentifierKeyword::Unknown;
  }

  if (metaschema.value().defines("$id")) {
    return IdentifierKeyword::Modern;
  }

  if (metaschema.value().defines("id")) {
    return IdentifierKeyword::Legacy;
  }

  return IdentifierKeyword::Unknown;
}

static inline auto
resolve_identifier(const sourcemeta::core::JSON &document,
                   const sourcemeta::core::JSON::String &keyword,
                   const sourcemeta::core::URI &base,
                   std::unordered_set<std::string> &accumulator)
    -> std::optional<sourcemeta::core::URI> {
  const auto *identifier{document.try_at(keyword)};
  if (identifier == nullptr || !identifier->is_string()) {
    return std::nullopt;
  }

  try {
    sourcemeta::core::URI resolved{identifier->to_string()};
    resolved.resolve_from(base).canonicalize();
    accumulator.insert(resolved.recompose());
    return resolved;
  } catch (const sourcemeta::core::URIParseError &) {
    accumulator.insert(identifier->to_string());
    return std::nullopt;
  }
}

// Identifiers are stored canonicalized, as whoever asks for one may well have
// resolved it against a base URI first and so spell it differently than the
// document that declares it. Trying the identifier as given before parsing it
// as a URI keeps the common case free, exactly as `find_resolve_match` does
static inline auto
declares_identifier(const std::unordered_set<std::string> &identifiers,
                    const std::string &identifier) -> bool {
  if (identifiers.contains(identifier)) {
    return true;
  }

  const auto canonical{canonical_resolve_key(identifier)};
  return canonical != identifier && identifiers.contains(canonical);
}

// What a file claims at its root without framing it, for a file whose dialect
// cannot be resolved yet. Only one of `$id` and `id` identifies a resource and
// which one depends on the dialect, so a file that cannot say which gets to
// claim whichever of the two it spells. An identifier that a file merely
// embeds stays out, as reaching it would mean walking into places whose
// meaning depends on the very vocabularies that are not known here
static inline auto
unframed_root_identifier(const sourcemeta::core::JSON &document,
                         const std::string_view default_dialect,
                         const std::string &default_id) -> std::string {
  if (!document.is_object()) {
    return default_id;
  }

  sourcemeta::core::URI base{default_id};
  base.canonicalize();

  const auto *dialect{document.try_at("$schema")};
  const auto keyword{dialect != nullptr && dialect->is_string()
                         ? identifier_keyword(dialect->to_string())
                         : identifier_keyword(default_dialect)};

  std::unordered_set<std::string> accumulator;
  if (keyword != IdentifierKeyword::Legacy) {
    const auto modern{resolve_identifier(document, "$id", base, accumulator)};
    if (modern.has_value()) {
      return modern.value().recompose();
    }
  }

  if (keyword != IdentifierKeyword::Modern) {
    const auto legacy{resolve_identifier(document, "id", base, accumulator)};
    if (legacy.has_value()) {
      return legacy.value().recompose();
    }
  }

  return default_id;
}

// The one identity a file answers for, which is what it declares at its root.
// Framing settles it wherever the dialect in force can be resolved, and a file
// whose dialect cannot be resolved yet still declares whatever it declares, so
// what the root spells is read directly in that case. Nothing here vets the
// file, as what a file is worth is settled when something names what it
// declares
static inline auto
framed_root_identifier(const sourcemeta::core::JSON &document,
                       const sourcemeta::core::SchemaResolver &resolver,
                       const std::string_view default_dialect,
                       const std::string &default_id)
    -> std::optional<std::string> {
  try {
    const sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::Root,
        document,
        sourcemeta::core::schema_walker,
        resolver,
        default_dialect,
        default_id,
        sourcemeta::core::SchemaFrame::IdentifierMode::Fallback};
    const auto &root{frame.root()};
    if (root.empty()) {
      return std::nullopt;
    }

    return std::string{root};
  } catch (const sourcemeta::core::Error &) {
    // Nothing here vets the file, so whatever keeps framing from settling the
    // identity is not reported from here. A dialect that cannot be resolved
    // yet says nothing about the identifier written beside it, so what the
    // root spells is read without it, and whatever is genuinely wrong with the
    // file is reported when something names what it declares and the file is
    // read in full
    return std::nullopt;
  }
}

static inline auto
root_identity(const sourcemeta::core::JSON &document,
              const sourcemeta::core::SchemaResolver &resolver,
              const std::string_view default_dialect,
              const std::string &default_id) -> std::string {
  const auto framed{
      framed_root_identifier(document, resolver, default_dialect, default_id)};
  if (framed.has_value()) {
    return framed.value();
  }

  return unframed_root_identifier(document, default_dialect, default_id);
}

// How resolution names what it reached is a URI, and a `file://` one names a
// place on disk that whoever reads an error would rather see spelled as a path
static inline auto identifier_path(const std::string &identifier)
    -> std::filesystem::path {
  std::optional<sourcemeta::core::URI> uri;
  try {
    uri.emplace(identifier);
  } catch (const sourcemeta::core::URIParseError &) {
    return std::filesystem::path{identifier};
  }

  return uri.value().is_file() ? uri.value().to_path()
                               : std::filesystem::path{identifier};
}

// The same question asked where a guess will not do: a URI that names no place
// on disk is better left unanswered than spelled as a path that leads nowhere
static inline auto identifier_file(const std::string &identifier)
    -> std::optional<std::filesystem::path> {
  std::optional<sourcemeta::core::URI> uri;
  try {
    uri.emplace(identifier);
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }

  if (!uri.value().is_file()) {
    return std::nullopt;
  }

  return uri.value().to_path();
}

class CustomResolver {
public:
  CustomResolver(
      const sourcemeta::core::Options &options,
      const std::optional<sourcemeta::blaze::Configuration> &configuration,
      const bool remote, const std::string_view default_dialect)
      : options_{options}, configuration_{configuration},
        canonical_resolve_{
            configuration.has_value()
                ? canonical_resolve_map(configuration->resolve)
                : std::unordered_map<std::string, std::string>{}},
        default_dialect_{default_dialect}, remote_{remote} {
    if (options.contains("resolve")) {
      // Reading a file end to end is what vets it, and resolution only ever
      // needs the files that something names, so an input is recorded here by
      // the one identity it answers for and read again on demand. Settling
      // that identity never asks the network, so that a file indexed before
      // the one declaring its dialect goes by what its own root spells rather
      // than by what a fetch for that dialect would turn up. An index that is
      // complete before any resolution runs is then what keeps the network
      // from ever shadowing a schema the user supplied locally
      const auto allow_remote{this->remote_};
      this->remote_ = false;
      this->indexing_ = true;

      for (auto &entry : for_each_json(options.at("resolve"), options)) {
        this->index_entry(entry, default_dialect);
      }

      this->indexing_ = false;
      this->remote_ = allow_remote;
    }

    if (this->configuration_.has_value()) {
      for (const auto &[dependency_uri, dependency_path] :
           this->configuration_.value().dependencies) {
        if (!std::filesystem::exists(dependency_path)) {
          continue;
        }

        auto schema{sourcemeta::core::read_json(dependency_path)};
        if (!schema.is_object() && !schema.is_boolean()) {
          continue;
        }

        // An installed dependency answers both to the identifier it declares
        // and to the URI it was imported under, as whoever wrote the
        // configuration may well name it either way
        const auto declared{canonical_resolve_key(root_identity(
            schema, std::ref(*this), default_dialect, dependency_uri))};
        this->index_.emplace(declared,
                             Leaf{.path = dependency_path,
                                  .display_name = dependency_path.string(),
                                  .kind = LeafKind::Schema,
                                  .answers_to_location = false});
        this->index_.emplace(canonical_resolve_key(dependency_uri),
                             Leaf{.path = dependency_path,
                                  .display_name = dependency_path.string(),
                                  .kind = LeafKind::Schema,
                                  .answers_to_location = false});
      }
    }
  }

  // Prevent accidental copies, as every schema this imported would come
  // along. Passing this resolver by value to anything that takes a
  // sourcemeta::core::SchemaResolver would do exactly that
  CustomResolver(const CustomResolver &) = delete;
  auto operator=(const CustomResolver &) -> CustomResolver & = delete;
  CustomResolver(CustomResolver &&) = default;
  auto operator=(CustomResolver &&) -> CustomResolver & = delete;
  ~CustomResolver() = default;

  // What a leaf of the resolution context holds, which decides which half of
  // the resolver may ever answer with it
  enum class LeafKind : std::uint8_t { Schema, OpenAPI };

  // Where an indexed input is read back from when something finally names what
  // it declares. An input that came from standard input has nowhere to be read
  // back from, so what it held is kept instead
  struct Leaf {
    std::filesystem::path path;
    std::string display_name;
    LeafKind kind{LeafKind::Schema};
    // Whether the file answers to where it sits as well as to what it
    // declares, which every input the user named does. An installed dependency
    // does not, as where it sits is a vendor directory this resolver put it in
    // and the configuration already says which URI it stands for
    bool answers_to_location{true};
    std::shared_ptr<InputJSON> retained{nullptr};
  };

  // How much of a file an import is allowed to read. Reading it end to end is
  // what vets it, so that is what every import aims for. Staging settles for
  // the resource the file declares at its root, which only asks for the root's
  // own dialect chain to terminate in something already known
  enum class ImportMode : std::uint8_t { Complete, Root };

  auto add(const ImportMode mode, const sourcemeta::core::JSON &schema,
           const std::filesystem::path &origin,
           const std::string_view default_dialect = "",
           const std::string_view default_id = "",
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr,
           const sourcemeta::core::PointerPositionTracker *positions = nullptr)
      -> bool {
    assert(schema.is_object() || schema.is_boolean());

    // Framing the whole document is what vets it, from the vocabularies every
    // resource declares to the anchors it collides on, so a complete import
    // keeps the analysis as wide as the file. What gets registered does not
    const sourcemeta::core::SchemaFrame frame{
        mode == ImportMode::Complete
            ? sourcemeta::core::SchemaFrame::Mode::References
            : sourcemeta::core::SchemaFrame::Mode::Root,
        schema,
        sourcemeta::core::schema_walker,
        std::ref(*this),
        default_dialect,
        default_id};

    bool added_any_schema{false};
    frame.for_each_resource(
        [this, &schema, &frame, &origin, &callback, &positions,
         &added_any_schema](
            const std::string_view uri,
            const sourcemeta::core::SchemaFrame::Location &entry) -> void {
          // Reject a resource whose vocabularies we cannot make sense of
          // upfront, rather than at the point some consumer relies on them
          const auto &subschema_vocabularies{
              frame.vocabularies(entry, std::ref(*this))};

          // A file stands for the single schema it declares. A resource that
          // the schema merely embeds is reachable from within that schema,
          // and answering for it on its own would hand back a schema that the
          // user never supplied as one. Remember it all the same, as knowing
          // where an identifier sits is what tells someone who names it from
          // the outside why nothing came back
          if (!entry.pointer.empty()) {
            auto location{sourcemeta::core::to_pointer(entry.pointer)};
            const auto position{
                positions == nullptr ? std::nullopt : positions->get(location)};
            this->embedded_.emplace(
                std::string{uri},
                EmbeddedResource{.origin = origin,
                                 .location = std::move(location),
                                 .position =
                                     position.has_value()
                                         ? std::make_optional(std::make_pair(
                                               std::get<0>(position.value()),
                                               std::get<1>(position.value())))
                                         : std::nullopt});
            return;
          }

          auto subschema{sourcemeta::core::get(schema, entry.pointer)};
          // Fully resolve the dialect and identifier, otherwise the consumer
          // might have no idea what to do with them. Neither keyword has
          // anywhere to go in a boolean schema, and a dialect that reserves
          // neither, as OpenAPI 3.0 does, admits no schema that writes one.
          // Such a schema is registered as it stands, and whoever reads it
          // names its dialect from the outside
          if (subschema.is_object()) {
            if (dialect_defines(subschema_vocabularies, "$schema")) {
              subschema.assign("$schema",
                               sourcemeta::core::JSON{entry.dialect});
            }

            if (dialect_defines(subschema_vocabularies,
                                sourcemeta::core::schema_identifier_keyword(
                                    entry.base_dialect))) {
              sourcemeta::core::schema_reidentify(subschema, uri,
                                                  entry.base_dialect);
            }
          }

          const std::string identifier{uri};
          const auto result{this->schemas_.emplace(identifier, subschema)};
          if (!result.second && result.first->second != subschema) {
            const auto other{this->origins_.find(identifier)};
            assert(other != this->origins_.cend());
            throw SchemaIdentifierConflictError{
                identifier, sourcemeta::core::to_pointer(entry.pointer),
                other->second.first, other->second.second};
          }

          this->origins_.emplace(
              identifier, std::make_pair(origin, sourcemeta::core::to_pointer(
                                                     entry.pointer)));

          if (callback) {
            callback(identifier);
          }

          added_any_schema = true;
        });

    return added_any_schema;
  }

  // An OpenAPI Description is never a schema, so this hands one back under no
  // circumstance. What a fetch turns up that is one is deposited among the
  // descriptions instead, where only the resolver below can reach it
  auto operator()(std::string_view identifier)
      -> sourcemeta::core::SchemaResolverResult {
    const std::string string_identifier{identifier};
    const auto mapped_result = this->configuration_.and_then(
        [this,
         &string_identifier](const sourcemeta::blaze::Configuration &config)
            -> std::optional<std::string> {
          return resolve_map_uri(this->canonical_resolve_, config.base_path,
                                 string_identifier);
        });
    const std::string &target{mapped_result.has_value() ? mapped_result.value()
                                                        : string_identifier};
    if (mapped_result.has_value()) {
      LOG_DEBUG(this->options_) << "Resolving " << identifier << " as "
                                << target << " given the configuration file\n";
    }

    const auto match{this->schemas_.find(target)};
    if (match != this->schemas_.cend()) {
      return match->second;
    }

    // An OpenAPI description is not a schema, so what was sorted among them is
    // unavailable here
    if (this->descriptions_.contains(target)) {
      return std::nullopt;
    }

    // An indexed input has not been read yet, so reading it is what makes it
    // answerable, and only the one file that answers to this identifier gets
    // read. Nothing is handed back where reading it settles on nothing at all,
    // as an identifier the user supplied locally must never reach the network
    // on the strength of this resolver not having read it yet
    const auto leaf{this->index_.find(target)};
    if (leaf != this->index_.cend()) {
      if (leaf->second.kind != LeafKind::Schema) {
        return std::nullopt;
      }

      // Reading this very file is already under way further up, which is what
      // a chain of dialects reaching back to one of its own links looks like.
      // Reading it again would never end, and what it holds is answer enough
      // for settling the dialect that asked for it, so it is handed over as it
      // was written rather than left unanswered
      if (!this->materialising_.emplace(target).second) {
        return this->stage(target, leaf->second);
      }

      const MaterialisingGuard guard{.pending = this->materialising_,
                                     .identifier = target};
      this->materialise(leaf->second);
      const auto materialised{this->schemas_.find(target)};
      if (materialised != this->schemas_.cend()) {
        return materialised->second;
      }

      // A leaf answers to the identity it was indexed under even where reading
      // the file settles on another, which is how an installed dependency
      // answers both to what it declares and to the URI it was imported under
      return this->alias_declared(target, leaf->second);
    }

    const auto cached{this->fetched_.find(target)};
    if (cached != this->fetched_.cend()) {
      return cached->second;
    }

    auto fetched{this->fetch_or_explain(target)};
    if (!fetched.has_value()) {
      // Nothing anywhere holds this. An indexed file that merely embeds it
      // is the one case worth explaining, as the usual advice to import it
      // would have the user import what they already did
      // Not while the inputs are still being indexed, as remote resolution is
      // deliberately held back there and a file that cannot resolve yet is
      // meant to go by what its own root spells rather than be given up on
      if (!this->indexing_) {
        const auto embedded{this->embedded_.find(target)};
        if (embedded != this->embedded_.cend()) {
          this->report_embedded(embedded->second, target);
        }

        this->report_indexed_embedded(target);
      }

      return fetched;
    }

    auto document{std::move(fetched).to_owned()};
    if (is_openapi_document(document)) {
      this->descriptions_.emplace(target, std::move(document));
      return std::nullopt;
    }

    // Working out whether a schema declares an identifier means framing it,
    // which resolves its metaschema through this very resolver. What was
    // retrieved is therefore kept before it is examined, so that a chain of
    // metaschemas that comes back to this document finds it here rather than
    // retrieving and framing it again at every step. A dialect that describes
    // itself resolves to itself that way, and one that loops through another
    // document is left for the frame to turn down, as the chain it follows is
    // the one place that knows how far it already went
    auto &stored{
        this->fetched_.emplace(target, std::move(document)).first->second};

    // Only a schema that declares no identifier of its own needs one
    const auto base_dialect{anonymous_base_dialect(stored, std::ref(*this))};
    if (base_dialect.has_value()) {
      sourcemeta::core::schema_reidentify(stored, string_identifier,
                                          base_dialect.value());
    }

    return stored;
  }

  // The other half of the split. What the URI names is handed back whether or
  // not it holds a description, as reading one that does not is how bundling
  // tells a reference that it landed on the wrong kind of thing, and it cannot
  // say so of a document it never sees. Nothing at all is handed back only
  // where the URI names nothing
  auto openapi(std::string_view identifier)
      -> sourcemeta::core::OpenAPIResolverResult {
    // What a configuration remaps an identifier to holds for a description just
    // as it does for a schema, so this is settled before anything is looked up
    const std::string string_identifier{identifier};
    const auto mapped_result = this->configuration_.and_then(
        [this,
         &string_identifier](const sourcemeta::blaze::Configuration &config)
            -> std::optional<std::string> {
          return resolve_map_uri(this->canonical_resolve_, config.base_path,
                                 string_identifier);
        });
    if (mapped_result.has_value()) {
      LOG_DEBUG(this->options_)
          << "Resolving " << identifier << " as " << mapped_result.value()
          << " given the configuration file\n";
    }

    const std::string target{canonical_resolve_key(
        mapped_result.has_value() ? mapped_result.value() : string_identifier)};

    const auto match{this->descriptions_.find(target)};
    if (match != this->descriptions_.cend()) {
      reject_unsupported_openapi(match->second, identifier_path(target));
      return match->second;
    }

    // A document that holds no description is still a document this URI names,
    // so wherever the schema half already keeps one, that is what answers here
    const auto imported{this->schemas_.find(target)};
    if (imported != this->schemas_.cend()) {
      return imported->second;
    }

    // An indexed input has not been read yet, and what a URI names is handed
    // back whether or not it holds a description, so either half of the index
    // answers here once the file it points at has been read
    const auto leaf{this->index_.find(target)};
    if (leaf != this->index_.cend() &&
        this->materialising_.emplace(target).second) {
      const MaterialisingGuard guard{.pending = this->materialising_,
                                     .identifier = target};
      this->materialise(leaf->second);

      const auto described{this->descriptions_.find(target)};
      if (described != this->descriptions_.cend()) {
        reject_unsupported_openapi(described->second, identifier_path(target));
        return described->second;
      }

      const auto materialised{this->schemas_.find(target)};
      if (materialised != this->schemas_.cend()) {
        return materialised->second;
      }
    }

    const auto cached{this->fetched_.find(target)};
    if (cached != this->fetched_.cend()) {
      return cached->second;
    }

    auto fetched{fetch_schema(this->options_, target, this->remote_)};
    if (!fetched.has_value()) {
      return std::nullopt;
    }

    auto document{std::move(fetched).to_owned()};
    if (!is_openapi_document(document)) {
      return this->fetched_.emplace(target, std::move(document)).first->second;
    }

    const auto &stored{
        this->descriptions_.emplace(target, std::move(document)).first->second};
    reject_unsupported_openapi(stored, identifier_path(target));
    return stored;
  }

  // Where a place of a file sits, for a file that was read without keeping
  // track. Nothing is reported rather than guessed when it cannot be read back
  static auto read_position(const std::filesystem::path &path,
                            const sourcemeta::core::Pointer &location)
      -> std::optional<std::pair<std::uint64_t, std::uint64_t>> {
    sourcemeta::core::PointerPositionTracker positions;
    sourcemeta::core::JSON document{nullptr};
    try {
      sourcemeta::core::read_yaml_or_json(path, document, std::ref(positions));
    } catch (...) {
      return std::nullopt;
    }

    const auto position{positions.get(location)};
    if (!position.has_value()) {
      return std::nullopt;
    }

    return std::make_pair(std::get<0>(position.value()),
                          std::get<1>(position.value()));
  }

  // Where an imported description was retrieved from, which is what a relative
  // identity of its own and its relative references resolve against. One that
  // answers to an identity it declares is still read from where it was found,
  // so the two part ways and only this one may be built upon
  [[nodiscard]] auto
  description_retrieval(const std::string_view identifier) const
      -> std::optional<std::string> {
    const auto match{
        this->description_retrievals_.find(canonical_resolve_key(identifier))};
    if (match == this->description_retrievals_.cend()) {
      return std::nullopt;
    }

    return match->second;
  }

  // A Schema Object that a description holds declares its identifier within
  // that description, so nothing outside it ever answers to that name. What
  // reveals those identifiers is framing, and importing a description
  // deliberately does not frame one, so this looks only once both halves have
  // already failed to answer, where knowing beats what it costs to find out
  auto report_description_resource(const std::string_view identifier) -> void {
    // An indexed description that nothing named has not been read yet, and one
    // of those is as likely to hold this identifier as one that has, so this
    // is where they are all finally read. Already a failing path, so what it
    // costs to find out is worth knowing, and whatever keeps one of them from
    // being read is not what we came to report
    for (const auto &entry : this->index_) {
      if (entry.second.kind != LeafKind::OpenAPI ||
          this->description_origins_.contains(entry.first) ||
          !this->materialising_.emplace(entry.first).second) {
        continue;
      }

      const MaterialisingGuard guard{.pending = this->materialising_,
                                     .identifier = entry.first};
      try {
        this->materialise(entry.second);
      } catch (const sourcemeta::core::Error &) {
        continue;
      }
    }

    // The same description answers to every identity it was registered under,
    // so what it was read from is what tells one apart from the next
    std::vector<std::filesystem::path> seen;
    for (const auto &[identity, description] : this->descriptions_) {
      const auto origin{this->description_origins_.find(identity)};
      const auto path{origin == this->description_origins_.cend()
                          ? identifier_path(identity)
                          : origin->second};
      if (std::ranges::find(seen, path) != seen.cend()) {
        continue;
      }

      seen.push_back(path);
      const auto retrieval{this->description_retrieval(identity)};
      const auto &base{retrieval.has_value() ? retrieval.value() : identity};

      std::optional<sourcemeta::core::OpenAPIFrame> frame;
      try {
        frame.emplace(description, sourcemeta::core::schema_walker,
                      std::ref(*this), base);
      } catch (...) {
        // Whatever keeps a description from being framed is not what we came
        // to report, and it has nothing to say about this identifier either
        continue;
      }

      const auto match{frame.value().schemas().traverse(identifier)};
      if (!match.has_value()) {
        continue;
      }

      auto location{sourcemeta::core::to_pointer(match.value().get().pointer)};

      // Importing a description keeps the document rather than where each part
      // of it was written, so where to point is read back here, on a path that
      // ends in a failure either way
      const auto position{read_position(path, location)};
      if (position.has_value()) {
        throw PositionError<
            sourcemeta::core::FileError<OpenAPIEmbeddedResourceError>>(
            position.value().first, position.value().second, path,
            std::string{identifier}, std::move(location));
      }

      throw sourcemeta::core::FileError<OpenAPIEmbeddedResourceError>(
          path, std::string{identifier}, std::move(location));
    }
  }

  // Bundling a document that resolution went and got reports a place within
  // that document rather than within whatever asked for it, so naming the
  // file the caller was reading would point at a place it does not hold.
  // Where the document came from is known only here, and importing keeps the
  // document rather than where each part of it was written, so where to point
  // is read back from that file
  [[noreturn]] auto report_schema_container(
      const std::string_view identifier,
      const sourcemeta::core::SchemaContainerError &error) const -> void {
    // Resolution goes wherever a configuration remaps the identifier to, so
    // what names the document is that rather than what the caller asked by
    const auto target{this->resolution_target(std::string{identifier})};
    const auto origin{this->origins_.find(target)};
    const auto file{origin != this->origins_.cend()
                        ? std::make_optional(origin->second.first)
                        : identifier_file(target)};

    // A document that was fetched from somewhere that is not a place on disk
    // is named by the URL it came from, as there is no file to read a position
    // out of. Reading one out of whatever asked for the document would answer
    // with a place that document never had, which is worse than saying
    // nothing, as a line and a column read as the one thing worth opening
    if (!file.has_value()) {
      throw URLError<sourcemeta::core::SchemaContainerError>(target, error);
    }

    // Importing keeps the document rather than where each part of it was
    // written, so where to point is read back from the file it came from
    const auto position{read_position(file.value(), error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::SchemaContainerError>>(
          position.value().first, position.value().second, file.value(), error);
    }

    throw sourcemeta::core::FileError<sourcemeta::core::SchemaContainerError>(
        file.value(), error);
  }

private:
  // Where resolution goes looking for an identifier, which is whatever a
  // configuration remaps it to or else the identifier as it stands. Asked the
  // same way resolution asks it, so that what answered and what is reported
  // as having answered cannot disagree
  [[nodiscard]] auto resolution_target(const std::string &identifier) const
      -> std::string {
    const auto mapped{this->configuration_.and_then(
        [this, &identifier](const sourcemeta::blaze::Configuration &config)
            -> std::optional<std::string> {
          return resolve_map_uri(this->canonical_resolve_, config.base_path,
                                 identifier);
        })};
    return mapped.has_value() ? mapped.value() : identifier;
  }

  // A description answers to the `$self` it declares, and to where it came
  // from either way, mirroring how an installed dependency is registered both
  // by the identifier it declares and by the URI it was imported under
  auto import_description(const InputJSON &entry) -> void {
    const auto retrieval{sourcemeta::jsonschema::default_id(entry)};
    LOG_DEBUG(this->options_)
        << "Importing OpenAPI description into the resolution context: "
        << retrieval << "\n";

    const auto self{openapi_self_identity(entry.second, retrieval)};
    if (self.has_value()) {
      const auto identity{canonical_resolve_key(self.value())};
      this->register_description(identity, entry);
      this->description_retrievals_.emplace(identity, retrieval);
    }

    const auto identity{canonical_resolve_key(retrieval)};
    this->register_description(identity, entry);
    this->description_retrievals_.emplace(identity, retrieval);
  }

  // Two descriptions that answer to one identifier leave which of them a
  // reference reaches to the order they happened to be given in, so this is
  // reported rather than settled by whichever arrived first
  auto register_description(const std::string &identifier,
                            const InputJSON &entry) -> void {
    const auto result{this->descriptions_.emplace(identifier, entry.second)};
    if (!result.second && result.first->second != entry.second) {
      const auto other{this->description_origins_.find(identifier)};
      assert(other != this->description_origins_.cend());
      throw sourcemeta::core::FileError<OpenAPIIdentifierConflictError>(
          entry.resolution_base, identifier, other->second);
    }

    if (result.second) {
      this->description_origins_.emplace(identifier, entry.resolution_base);
    }
  }

  // What is currently being read, so that a dialect chain coming back to a
  // file already being read is left unanswered rather than read again forever
  struct MaterialisingGuard {
    std::unordered_set<std::string> &pending;
    const std::string &identifier;
    ~MaterialisingGuard() { this->pending.erase(this->identifier); }
  };

  // The one identity an input answers for, settled without reading anything
  // else. A description is never framed, as what it holds is the business of
  // the OpenAPI half, and its identity is the one it declares along with where
  // it was read from
  auto index_entry(InputJSON &entry, const std::string_view default_dialect)
      -> void {
    if (is_openapi_document(entry.second)) {
      reject_unsupported_openapi(entry.second, entry.resolution_base,
                                 &entry.positions);
      const auto retrieval{sourcemeta::jsonschema::default_id(entry)};
      const auto self{openapi_self_identity(entry.second, retrieval)};
      if (self.has_value()) {
        this->register_leaf(canonical_resolve_key(self.value()), entry,
                            LeafKind::OpenAPI);
      }

      this->register_leaf(canonical_resolve_key(retrieval), entry,
                          LeafKind::OpenAPI);
      return;
    }

    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base,
          sourcemeta::core::StaticString{
              "The file you provided does not represent a valid JSON Schema"});
    }

    this->register_leaf(canonical_resolve_key(root_identity(
                            entry.second, std::ref(*this), default_dialect,
                            sourcemeta::jsonschema::default_id(entry))),
                        entry, LeafKind::Schema);
  }

  // Two inputs that answer to one identifier leave which of them a reference
  // reaches to the order they happened to be given in, so this is reported
  // rather than settled by whichever arrived first. Two that hold the very
  // same document are no such ambiguity, and settling that is what reading the
  // one already indexed is worth
  auto register_leaf(const std::string &identity, InputJSON &entry,
                     const LeafKind kind) -> void {
    const auto match{this->index_.find(identity)};
    if (match != this->index_.cend()) {
      if (match->second.path == entry.resolution_base ||
          indexed_contents(match->second) == entry.second) {
        return;
      }

      if (kind == LeafKind::OpenAPI) {
        throw sourcemeta::core::FileError<OpenAPIIdentifierConflictError>(
            entry.resolution_base, identity, match->second.path);
      }

      // A file answers for what it declares at its root, so the root is the
      // one place a conflict between two of them can be pointed at
      const sourcemeta::core::Pointer location;
      const auto position{entry.positions.get(location)};
      if (position.has_value()) {
        throw PositionError<
            sourcemeta::core::FileError<SchemaIdentifierConflictError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, identity, location, match->second.path,
            sourcemeta::core::Pointer{});
      }

      throw sourcemeta::core::FileError<SchemaIdentifierConflictError>(
          entry.resolution_base, identity, location, match->second.path,
          sourcemeta::core::Pointer{});
    }

    // Nothing can be read back for an input that came from standard input, so
    // what it held is the one copy of it there will ever be
    this->index_.emplace(
        identity,
        Leaf{.path = entry.resolution_base,
             .display_name = entry.first,
             .kind = kind,
             .retained = entry.from_stdin
                             ? std::make_shared<InputJSON>(std::move(entry))
                             : nullptr});
  }

  // Where an identifier that nothing answers for actually sits, for the one
  // case worth explaining: a resource that an indexed file merely embeds.
  // Finding it means reading the files that were indexed and not read yet, so
  // this only ever runs once resolution has already failed, where knowing
  // beats what it costs to find out. Whatever keeps one of those files from
  // being read is not what we came to report, and it has nothing to say about
  // this identifier either
  auto report_indexed_embedded(const std::string &identifier) -> void {
    for (const auto &entry : this->index_) {
      if (entry.second.kind != LeafKind::Schema ||
          this->origins_.contains(entry.first)) {
        continue;
      }

      if (this->materialising_.emplace(entry.first).second) {
        const MaterialisingGuard guard{.pending = this->materialising_,
                                       .identifier = entry.first};
        try {
          this->materialise(entry.second);
        } catch (const sourcemeta::core::Error &) {
          continue;
        }
      }

      const auto embedded{this->embedded_.find(identifier)};
      if (embedded != this->embedded_.cend()) {
        this->report_embedded(embedded->second, identifier);
      }
    }
  }

  // What a file holds, for a file that is already being read further up. Kept
  // apart from what reading it in full registers, as this has been through
  // none of that and must never be mistaken for it
  auto stage(const std::string &identity, const Leaf &leaf)
      -> sourcemeta::core::SchemaResolverResult {
    const auto staged{this->staged_.find(identity)};
    if (staged != this->staged_.cend()) {
      return staged->second;
    }

    auto contents{indexed_contents(leaf)};
    if (!contents.is_object() && !contents.is_boolean()) {
      return std::nullopt;
    }

    return this->staged_.emplace(identity, std::move(contents)).first->second;
  }

  // What reading a file registered it as, where that is not the identity it
  // was indexed under. Only a resource the file declares at its root counts,
  // as that is the one a file ever answers for
  auto alias_declared(const std::string &identity, const Leaf &leaf)
      -> sourcemeta::core::SchemaResolverResult {
    std::string declared;
    for (const auto &origin : this->origins_) {
      if (origin.second.first == leaf.path && origin.second.second.empty()) {
        declared = origin.first;
        break;
      }
    }

    if (declared.empty()) {
      return std::nullopt;
    }

    const auto match{this->schemas_.find(declared)};
    if (match == this->schemas_.cend()) {
      return std::nullopt;
    }

    this->origins_.emplace(
        identity, std::make_pair(leaf.path, sourcemeta::core::Pointer{}));
    return this->schemas_.emplace(identity, match->second).first->second;
  }

  // What an indexed input holds, for the one question that cannot be answered
  // without looking: whether two inputs claiming one identity hold the same
  // document. Nothing at all where it cannot be read, as a file that cannot be
  // read is not one that agrees with anything
  [[nodiscard]] static auto indexed_contents(const Leaf &leaf)
      -> sourcemeta::core::JSON {
    if (leaf.retained != nullptr) {
      return leaf.retained->second;
    }

    try {
      return sourcemeta::core::read_yaml_or_json(leaf.path);
    } catch (...) {
      return sourcemeta::core::JSON{nullptr};
    }
  }

  // Reading a file end to end is what vets it and what settles its dialect and
  // identifier, and only a file that something names is worth either, so this
  // is where an indexed input is finally read. What it declares is registered
  // exactly as reading every input up front would have registered it
  auto materialise(const Leaf &leaf) -> void {
    // Everything below already names the file it is about. A command is
    // working on another file and decorates a bare error with that one, so
    // what comes out of here travels wrapped, out of reach of those handlers
    try {
      this->read_and_import(leaf);
    } catch (const ForeignFileError &) {
      // One file read on the way to another is still one file to name, so a
      // chain of them keeps naming the file that the error is actually about
      throw;
    } catch (...) {
      throw ForeignFileError{std::current_exception()};
    }
  }

  auto read_and_import(const Leaf &leaf) -> void {
    if (leaf.retained != nullptr) {
      this->import_entry(*leaf.retained, this->default_dialect_,
                         ImportMode::Complete, leaf.answers_to_location);
      return;
    }

    InputJSON entry{.first = leaf.display_name,
                    .resolution_base = leaf.path,
                    .second = sourcemeta::core::JSON{nullptr},
                    .positions = {},
                    .property_storage = nullptr};
    sourcemeta::core::read_yaml_or_json(leaf.path, entry.second,
                                        std::ref(entry.positions));
    this->import_entry(entry, this->default_dialect_, ImportMode::Complete,
                       leaf.answers_to_location);
  }

  // What an imported file embeds rather than declares at its root. Never
  // answered with, as that is the point, but kept so that naming one of these
  // from the outside gets an explanation rather than a shrug
  struct EmbeddedResource {
    std::filesystem::path origin;
    sourcemeta::core::Pointer location;
    std::optional<std::pair<std::uint64_t, std::uint64_t>> position;
  };

  // Fetching, with one thing added: a remote that says it does not have this
  // settles the question just as firmly as finding nothing locally does, so
  // an input that merely embeds the identifier is worth pointing at rather
  // than letting the status stand as the whole answer
  auto fetch_or_explain(const std::string &target)
      -> sourcemeta::core::SchemaResolverResult {
    try {
      return fetch_schema(this->options_, target, this->remote_);
    } catch (const sourcemeta::core::HTTPStatusError &error) {
      if (this->indexing_ ||
          error.status() != sourcemeta::core::HTTP_STATUS_NOT_FOUND) {
        throw;
      }

      const auto embedded{this->embedded_.find(target)};
      if (embedded != this->embedded_.cend()) {
        this->report_embedded(embedded->second, target);
      }

      // An indexed input that nothing named has not been read yet, and one of
      // those is as likely to embed this identifier as one that has
      this->report_indexed_embedded(target);
      throw;
    }
  }

  // Naming a resource that an input merely embeds gets an explanation rather
  // than the advice to import what is already imported
  [[noreturn]] auto report_embedded(const EmbeddedResource &embedded,
                                    const std::string &identifier) const
      -> void {
    if (embedded.position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<SchemaEmbeddedResourceError>>(
          embedded.position.value().first, embedded.position.value().second,
          embedded.origin, identifier, embedded.location);
    }

    throw sourcemeta::core::FileError<SchemaEmbeddedResourceError>(
        embedded.origin, identifier, embedded.location);
  }

  auto import_entry(const InputJSON &entry,
                    const std::string_view default_dialect,
                    const ImportMode mode,
                    const bool answers_to_location = true) -> void {
    // What a description holds is the business of the OpenAPI resolver, and
    // framing it as a schema would both fail and register nonsense. Only a
    // revision we can read is taken, as one we cannot is no more a description
    // we can answer for than a schema
    if (is_openapi_document(entry.second)) {
      reject_unsupported_openapi(entry.second, entry.resolution_base);
      this->import_description(entry);
      return;
    }

    if (mode == ImportMode::Complete) {
      LOG_DEBUG(this->options_)
          << "Detecting schema resources from file: " << entry.first << "\n";
    }

    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base,
          sourcemeta::core::StaticString{
              "The file you provided does not represent a valid JSON Schema"});
    }

    try {
      const auto result = this->add(
          mode, entry.second, entry.resolution_base, default_dialect,
          answers_to_location ? sourcemeta::jsonschema::default_id(entry)
                              : std::string{},
          [this](const auto &identifier) {
            LOG_DEBUG(this->options_)
                << "Importing schema into the resolution context: "
                << identifier << "\n";
          },
          &entry.positions);
      // Staging a root is only ever half of an import, so a file that
      // declares nothing is worth reporting once the complete read says so
      if (!result && mode == ImportMode::Complete) {
        LOG_WARNING() << "No schema resources were imported from this file\n"
                      << "  at " << entry.first << "\n"
                      << "Are you sure this schema sets any identifiers?\n";
      }
    } catch (const SchemaIdentifierConflictError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<
            sourcemeta::core::FileError<SchemaIdentifierConflictError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error.identifier(), error.location(),
            error.other_path(), error.other());
      }

      throw sourcemeta::core::FileError<SchemaIdentifierConflictError>(
          entry.resolution_base, error.identifier(), error.location(),
          error.other_path(), error.other());
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>(entry.resolution_base,
                                                        error);
    } catch (const sourcemeta::core::SchemaDialectImpossibleError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaDialectImpossibleError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaDialectImpossibleError>(entry.resolution_base,
                                                          error);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaReferenceError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(
          entry.resolution_base);
    } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(entry.resolution_base,
                                                   error);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base, error);
    }
  }

  std::map<std::string, EmbeddedResource> embedded_{};
  std::map<std::string, sourcemeta::core::JSON> schemas_{};
  // Kept wholly apart from the schemas above, so that neither resolver can
  // reach what the other answers for
  std::map<std::string, sourcemeta::core::JSON> descriptions_{};
  std::map<std::string, std::filesystem::path> description_origins_{};
  // What a description was read from, kept apart from the identities it
  // answers to, as a relative identity resolves against this rather than
  // against itself
  std::map<std::string, std::string> description_retrievals_{};
  // What resolution has already retrieved and found not to be a description,
  // so that one URL is fetched once however many times it is asked for
  std::map<std::string, sourcemeta::core::JSON> fetched_{};
  std::map<std::string,
           std::pair<std::filesystem::path, sourcemeta::core::Pointer>>
      origins_{};
  // Where every input answers from, settled before any resolution runs so that
  // an identifier the user supplied locally is never looked for anywhere else.
  // What each one holds is read back from here on demand, as reading a file is
  // only worth it once something names what it declares
  std::map<std::string, Leaf> index_{};
  // What a file holds, for one being read further up the chain that asked for
  // it. Never what reading a file in full registers, which the schemas above
  // are alone in holding
  std::map<std::string, sourcemeta::core::JSON> staged_{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::blaze::Configuration> configuration_;
  const std::unordered_map<std::string, std::string> canonical_resolve_;
  const std::string default_dialect_;
  bool remote_{false};
  // Whether the `--resolve` inputs are still being indexed, during which a
  // miss means "not yet" rather than "nowhere"
  bool indexing_{false};
  std::unordered_set<std::string> materialising_{};
};

using ResolverCacheKey = std::pair<bool, std::string>;

// Both halves of the split answer from one of these, so that what the schemas
// and what the descriptions were sorted into stays one set rather than two
// that disagree
inline auto resolver_instance(
    const sourcemeta::core::Options &options, const bool remote,
    const std::string_view default_dialect,
    const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> CustomResolver & {
  // One resolver per thread, as resolution reads files and registers what
  // they declare as it goes, and a context several threads register into at
  // once is one none of them can rely on. Indexing an input is cheap enough
  // for each thread to do its own, and a thread only ever reads the files
  // that the work it was given names
  thread_local std::map<ResolverCacheKey, CustomResolver> resolver_cache;
  const ResolverCacheKey cache_key{remote, std::string{default_dialect}};
  const auto match{resolver_cache.find(cache_key)};
  if (match != resolver_cache.cend()) {
    return match->second;
  }

  return resolver_cache
      .emplace(std::piecewise_construct, std::forward_as_tuple(cache_key),
               std::forward_as_tuple(options, configuration, remote,
                                     default_dialect))
      .first->second;
}

inline auto
resolver(const sourcemeta::core::Options &options, const bool remote,
         const std::string_view default_dialect,
         const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const sourcemeta::core::SchemaResolver & {
  // What callers get is a handle that refers back to the cached resolver,
  // as the resolver itself must never be copied into the callee
  thread_local std::map<ResolverCacheKey, sourcemeta::core::SchemaResolver>
      handle_cache;
  const ResolverCacheKey cache_key{remote, std::string{default_dialect}};

  const auto handle{handle_cache.find(cache_key)};
  if (handle != handle_cache.cend()) {
    return handle->second;
  }

  return handle_cache
      .emplace(cache_key, std::ref(resolver_instance(
                              options, remote, default_dialect, configuration)))
      .first->second;
}

// The OpenAPI half, which answers from the same instance as the schema half
// above. What a URI names is handed back whether or not it holds a description,
// as reading one that does not is how bundling tells a reference that it landed
// on the wrong kind of thing
inline auto openapi_resolver(
    const sourcemeta::core::Options &options, const bool remote,
    const std::string_view default_dialect,
    const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const sourcemeta::core::OpenAPIResolver & {
  thread_local std::map<ResolverCacheKey, sourcemeta::core::OpenAPIResolver>
      handle_cache;
  const ResolverCacheKey cache_key{remote, std::string{default_dialect}};

  const auto handle{handle_cache.find(cache_key)};
  if (handle != handle_cache.cend()) {
    return handle->second;
  }

  auto &instance{
      resolver_instance(options, remote, default_dialect, configuration)};
  return handle_cache
      .emplace(cache_key,
               [&instance](const std::string_view identifier)
                   -> sourcemeta::core::OpenAPIResolverResult {
                 return instance.openapi(identifier);
               })
      .first->second;
}

} // namespace sourcemeta::jsonschema

#endif
