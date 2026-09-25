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

#include <cassert> // assert
#include <chrono>  // std::chrono::seconds
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
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

// OpenAPI Specification 3.2.1, Section 4.1 lets a document name itself with
// `$self`, "which also serves as its base URI", resolved against wherever the
// document was retrieved from. RFC 3986 Section 5.2.2 never resolves a
// reference against a fragment, so one written there is no part of the base
static inline auto openapi_self_identity(const sourcemeta::core::JSON &document,
                                         const std::string &retrieval)
    -> std::optional<std::string> {
  if (!document.is_object()) {
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
        remote_{remote} {
    if (options.contains("resolve")) {
      const auto entries{for_each_json(options.at("resolve"), options)};
      std::vector<std::size_t> pending;
      pending.reserve(entries.size());
      for (std::size_t index = 0; index < entries.size(); index++) {
        pending.push_back(index);
      }

      // Importing a schema requires resolving its meta-schema, which may well
      // be another one of the schemas that the user is importing. Rather than
      // forcing the user to declare their files in dependency order, keep
      // retrying the ones that cannot resolve yet for as long as every pass
      // manages to import at least one more schema. Keep remote fetching
      // disabled while the locally provided schemas can make progress, so
      // that a schema imported before the local file that declares its
      // meta-schema resolves against that local file instead of triggering
      // a network fetch for it
      const auto allow_remote{this->remote_};

      this->remote_ = false;
      while (!pending.empty()) {
        std::vector<std::size_t> deferred;
        std::exception_ptr failure;

        for (const auto index : pending) {
          try {
            this->import_entry(entries[index], default_dialect);
          } catch (const sourcemeta::core::FileError<
                   sourcemeta::core::SchemaResolutionError> &) {
            if (!failure) {
              failure = std::current_exception();
            }

            LOG_DEBUG(options)
                << "Deferring import until the remaining schemas are "
                   "imported: "
                << entries[index].first << "\n";
            deferred.push_back(index);
          }
        }

        // Nothing can make progress anymore, so report the first failure,
        // which is exactly what the user would have seen if imports were
        // never retried. Note that when several entries remain stuck, the
        // one we report might be waiting on another stuck entry rather than
        // on the schema that is genuinely missing
        if (deferred.size() == pending.size()) {
          // Before giving up, let the remaining entries try their remote
          // fallback when the user enabled it
          if (allow_remote && !this->remote_) {
            // Remote fetching must still never shadow a schema that the user
            // supplied locally, so remember what the entries that are still
            // stuck declare before letting the network in. Entries that did
            // get imported are found among the imported schemas before this
            // ever comes into play
            for (const auto index : deferred) {
              this->collect_pending_identifiers(entries[index],
                                                default_dialect);
            }

            this->remote_ = true;
          } else {
            std::rethrow_exception(failure);
          }
        }

        pending = std::move(deferred);
      }

      this->pending_identifiers_.clear();
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

        try {
          this->add(schema, dependency_path, default_dialect);
        } catch (...) {
          continue;
        }

        if (this->schemas_.emplace(dependency_uri, schema).second) {
          this->origins_.emplace(
              dependency_uri,
              std::make_pair(dependency_path, sourcemeta::core::Pointer{}));
        }
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

  auto add(const sourcemeta::core::JSON &schema,
           const std::filesystem::path &origin,
           const std::string_view default_dialect = "",
           const std::string_view default_id = "",
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr) -> bool {
    assert(schema.is_object() || schema.is_boolean());

    // Framing the whole document is what vets it, from the vocabularies every
    // resource declares to the anchors it collides on, so the analysis stays
    // as wide as the file. What gets registered does not
    const sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::References,
        schema,
        sourcemeta::core::schema_walker,
        std::ref(*this),
        default_dialect,
        default_id};

    bool added_any_schema{false};
    frame.for_each_resource(
        [this, &schema, &frame, &origin, &callback, &added_any_schema](
            const std::string_view uri,
            const sourcemeta::core::SchemaFrame::Location &entry) -> void {
          // Reject a resource whose vocabularies we cannot make sense of
          // upfront, rather than at the point some consumer relies on them
          [[maybe_unused]] const auto &subschema_vocabularies{
              frame.vocabularies(entry, std::ref(*this))};

          // A file stands for the single schema it declares. A resource that
          // the schema merely embeds is reachable from within that schema,
          // and answering for it on its own would hand back a schema that the
          // user never supplied as one
          if (!entry.pointer.empty()) {
            return;
          }

          auto subschema{sourcemeta::core::get(schema, entry.pointer)};
          // Fully resolve the dialect and identifier, otherwise the
          // consumer might have no idea what to do with them
          subschema.assign("$schema", sourcemeta::core::JSON{entry.dialect});
          sourcemeta::core::schema_reidentify(subschema, uri,
                                              entry.base_dialect);

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

    if (this->remote_ &&
        declares_identifier(this->pending_identifiers_, target)) {
      return std::nullopt;
    }

    // An OpenAPI description is not a schema, so what was sorted among them is
    // unavailable here. Saying so is what tells a reference that goes looking
    // for one apart from a reference to something nobody supplied
    if (this->descriptions_.contains(target)) {
      LOG_VERBOSE(this->options_)
          << "Not available as a schema, as this was read as an OpenAPI "
             "description: "
          << target << "\n";
      return std::nullopt;
    }

    const auto cached{this->fetched_.find(target)};
    if (cached != this->fetched_.cend()) {
      return cached->second;
    }

    auto fetched{fetch_schema(this->options_, target, this->remote_)};
    if (!fetched.has_value()) {
      return fetched;
    }

    auto document{std::move(fetched).to_owned()};
    if (is_openapi_document(document)) {
      LOG_VERBOSE(this->options_)
          << "Not available as a schema, as this was read as an OpenAPI "
             "description: "
          << target << "\n";
      this->descriptions_.emplace(target, std::move(document));
      return std::nullopt;
    }

    // Only a schema that declares no identifier of its own needs one
    const auto base_dialect{anonymous_base_dialect(document, std::ref(*this))};
    if (base_dialect.has_value()) {
      sourcemeta::core::schema_reidentify(document, string_identifier,
                                          base_dialect.value());
    }

    return this->fetched_.emplace(target, std::move(document)).first->second;
  }

  // The other half of the split. An OpenAPI Description is what this hands
  // back and the only thing it ever does, so a schema that a fetch turns up
  // goes to the resolver above rather than being reported from here
  auto openapi(std::string_view identifier)
      -> sourcemeta::core::OpenAPIResolverResult {
    const std::string target{canonical_resolve_key(identifier)};

    const auto match{this->descriptions_.find(target)};
    if (match != this->descriptions_.cend()) {
      reject_unsupported_openapi(match->second, identifier_path(target));
      return match->second;
    }

    if (this->fetched_.contains(target)) {
      return std::nullopt;
    }

    auto fetched{fetch_schema(this->options_, target, this->remote_)};
    if (!fetched.has_value()) {
      return std::nullopt;
    }

    auto document{std::move(fetched).to_owned()};
    if (!is_openapi_document(document)) {
      this->fetched_.emplace(target, std::move(document));
      return std::nullopt;
    }

    const auto &stored{
        this->descriptions_.emplace(target, std::move(document)).first->second};
    reject_unsupported_openapi(stored, identifier_path(target));
    return stored;
  }

private:
  // Framing a schema is what reveals the identifiers it declares, but framing
  // needs its meta-schema resolved first, which is precisely what an entry
  // that cannot be imported is missing. Read the identifier it declares at
  // its own root instead, as that is the only claim that can be read without
  // knowing the dialect. An identifier that such an entry merely embeds stays
  // out, as reaching it would mean walking into places whose meaning depends
  // on the very vocabularies that are not known here, and mistaking the
  // instance an annotation carries for a schema resource would keep a schema
  // the user never supplied away from the network
  auto collect_pending_identifiers(const InputJSON &entry,
                                   const std::string_view default_dialect)
      -> void {
    sourcemeta::core::URI base{sourcemeta::jsonschema::default_id(entry)};
    base.canonicalize();
    this->pending_identifiers_.insert(base.recompose());

    if (!entry.second.is_object()) {
      return;
    }

    const auto *dialect{entry.second.try_at("$schema")};
    const auto keyword{dialect != nullptr && dialect->is_string()
                           ? identifier_keyword(dialect->to_string())
                           : identifier_keyword(default_dialect)};

    // Only one of `$id` and `id` identifies a resource, and which one depends
    // on the dialect. An entry that cannot be imported is one whose dialect
    // could not be resolved, so it gets to claim whichever of the two it
    // spells
    if (keyword != IdentifierKeyword::Legacy) {
      [[maybe_unused]] const auto modern{resolve_identifier(
          entry.second, "$id", base, this->pending_identifiers_)};
    }

    if (keyword != IdentifierKeyword::Modern) {
      [[maybe_unused]] const auto legacy{resolve_identifier(
          entry.second, "id", base, this->pending_identifiers_)};
    }
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
      this->register_description(canonical_resolve_key(self.value()), entry);
    }

    this->register_description(canonical_resolve_key(retrieval), entry);
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

  auto import_entry(const InputJSON &entry,
                    const std::string_view default_dialect) -> void {
    // What a description holds is the business of the OpenAPI resolver, and
    // framing it as a schema would both fail and register nonsense. Only a
    // revision we can read is taken, as one we cannot is no more a description
    // we can answer for than a schema
    if (is_openapi_document(entry.second)) {
      reject_unsupported_openapi(entry.second, entry.resolution_base);
      this->import_description(entry);
      return;
    }

    LOG_DEBUG(this->options_)
        << "Detecting schema resources from file: " << entry.first << "\n";

    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base,
          "The file you provided does not represent a valid JSON Schema");
    }

    try {
      const auto result =
          this->add(entry.second, entry.resolution_base, default_dialect,
                    sourcemeta::jsonschema::default_id(entry),
                    [this](const auto &identifier) {
                      LOG_DEBUG(this->options_)
                          << "Importing schema into the resolution context: "
                          << identifier << "\n";
                    });
      if (!result) {
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
          entry.resolution_base, error.identifier(), error.what());
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
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
          entry.resolution_base, error.identifier(), error.location(),
          error.what());
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
          sourcemeta::core::SchemaResolutionError>(
          entry.resolution_base, error.identifier(), error.what());
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base, error.what());
    }
  }

  std::map<std::string, sourcemeta::core::JSON> schemas_{};
  // Kept wholly apart from the schemas above, so that neither resolver can
  // reach what the other answers for
  std::map<std::string, sourcemeta::core::JSON> descriptions_{};
  std::map<std::string, std::filesystem::path> description_origins_{};
  // What resolution has already retrieved and found not to be a description,
  // so that one URL is fetched once however many times it is asked for
  std::map<std::string, sourcemeta::core::JSON> fetched_{};
  std::map<std::string,
           std::pair<std::filesystem::path, sourcemeta::core::Pointer>>
      origins_{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::blaze::Configuration> configuration_;
  const std::unordered_map<std::string, std::string> canonical_resolve_;
  bool remote_{false};
  std::unordered_set<std::string> pending_identifiers_{};
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
  static std::map<ResolverCacheKey, CustomResolver> resolver_cache;
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
  static std::map<ResolverCacheKey, sourcemeta::core::SchemaResolver>
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
// above and holds to the same separation: a schema is never reported from here
inline auto openapi_resolver(
    const sourcemeta::core::Options &options, const bool remote,
    const std::string_view default_dialect,
    const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const sourcemeta::core::OpenAPIResolver & {
  static std::map<ResolverCacheKey, sourcemeta::core::OpenAPIResolver>
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
