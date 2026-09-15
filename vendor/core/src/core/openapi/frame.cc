#include <sourcemeta/core/openapi.h>

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "document.h"
#include "helpers.h"
#include "info.h"

#include <algorithm>   // std::ranges::find
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <map>         // std::map
#include <memory>      // std::make_unique
#include <optional>    // std::optional
#include <set>         // std::set
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair, std::unreachable
#include <vector>      // std::vector

namespace {
using namespace std::string_view_literals;

// The base a location is keyed by is the key up to its fragment, which is why
// nothing repeats it on the entry itself. A parent is given as one of these
// keys rather than as a bare pointer, so that following it is a lookup in the
// same map rather than a key the reader has to rebuild
auto document_of(const sourcemeta::core::JSON::String &uri)
    -> sourcemeta::core::JSON::String {
  return sourcemeta::core::JSON::String{sourcemeta::core::take_until(uri, '#')};
}

auto parent_of(const std::map<sourcemeta::core::JSON::String,
                              sourcemeta::core::OpenAPILocation> &locations,
               const sourcemeta::core::JSON::String &uri,
               const sourcemeta::core::OpenAPILocation &location)
    -> sourcemeta::core::JSON {
  if (location.pointer.empty()) {
    return sourcemeta::core::JSON{nullptr};
  }

  const auto document{document_of(uri)};
  auto pointer{location.pointer};
  while (!pointer.empty()) {
    pointer.pop_back();
    auto candidate{sourcemeta::core::openapi_location_uri(document, pointer)};
    if (locations.contains(candidate)) {
      return sourcemeta::core::JSON{std::move(candidate)};
    }
  }

  return sourcemeta::core::JSON{document};
}

// Where a problem found once the walk is over belongs. A location says which
// base it is keyed by and where under it the Object sits, and a field hangs
// off that when the problem is with one rather than with the Object holding
// it
auto error_at(const std::map<sourcemeta::core::JSON::String,
                             sourcemeta::core::OpenAPILocation> &locations,
              const sourcemeta::core::JSON::String &location,
              const char *message,
              const sourcemeta::core::JSON::StringView field = {})
    -> sourcemeta::core::OpenAPIError {
  const auto match{locations.find(location)};
  auto pointer{match == locations.cend() ? sourcemeta::core::EMPTY_POINTER
                                         : match->second.pointer};
  if (!field.empty()) {
    pointer = pointer.concat(sourcemeta::core::JSON::String{field});
  }

  return {document_of(location), std::move(pointer), message};
}

auto error_at(const sourcemeta::core::OpenAPIWalk &walk,
              const sourcemeta::core::JSON::String &location,
              const char *message,
              const sourcemeta::core::JSON::StringView field = {})
    -> sourcemeta::core::OpenAPIError {
  return error_at(walk.locations, location, message, field);
}

// OpenAPI Specification 3.2.1, Section 4.22, of a Tag Object's `parent`:
// "The named tag MUST exist in the API description, and circular references
// between parent and child tags MUST NOT be used". A description spans every
// document it references, so a parent naming a tag this one does not declare
// is only missing where nothing is missing, and a cycle is a property of the
// tags held rather than of any one tag
auto check_tag_parents(
    const sourcemeta::core::OpenAPIWalk &walk,
    const std::map<sourcemeta::core::JSON::String,
                   sourcemeta::core::OpenAPILocation> &locations,
    const bool whole) -> void {
  std::map<sourcemeta::core::JSON::String, sourcemeta::core::JSON::String>
      parents;
  for (const auto &[location, edge] : walk.tag_parents) {
    if (whole && !walk.tag_names.contains(edge.second)) {
      throw error_at(locations, location,
                     "The Tag Object parent must name a tag the OpenAPI "
                     "Description declares",
                     "parent");
    }

    parents.insert_or_assign(edge.first, edge.second);
  }

  // Walking upward from each tag terminates at a tag with no parent unless the
  // chain comes back round, and a chain longer than the number of edges has
  // come back round
  for (const auto &[location, edge] : walk.tag_parents) {
    auto name{edge.first};
    for (std::size_t step = 0; step <= parents.size(); step += 1) {
      const auto next{parents.find(name)};
      if (next == parents.cend()) {
        break;
      }

      name = next->second;
      if (step == parents.size()) {
        throw error_at(locations, location,
                       "The Tag Object parents must not form a cycle",
                       "parent");
      }
    }
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.20: "The identified or reference
// operation MUST be unique, and in the case of an `operationId`, it MUST be
// resolved within the scope of the OpenAPI Description". Section 4.3.3
// recommends resolving one "considering all Operation Objects from all parsed
// documents", and only one document is ever parsed, which is why nothing is
// decided here unless the frame stands alone
auto check_operation_id_links(
    const sourcemeta::core::OpenAPIWalk &walk,
    const std::map<sourcemeta::core::JSON::String,
                   sourcemeta::core::OpenAPILocation> &locations) -> void {
  for (const auto &[location, identifier] : walk.operation_id_links) {
    // Section 4.8.20 goes on to say that an operation reached through a Path
    // Item referenced more than once "cannot be resolved unambiguously", and
    // that "in such ambiguous cases, the resulting behavior is
    // implementation-defined and MAY result in an error". So naming nothing at
    // all is the violation, and naming something twice over is not
    if (!walk.operation_ids.contains(identifier)) {
      throw error_at(locations, location,
                     "The Link Object operation identifier must name an "
                     "operation the OpenAPI Description declares",
                     "operationId");
    }
  }
}

auto info_json(const sourcemeta::core::OpenAPIInfo &info)
    -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign_assume_new("title", sourcemeta::core::JSON{info.title});
  result.assign_assume_new("version", sourcemeta::core::JSON{info.version});
  result.assign_assume_new("summary", sourcemeta::core::to_json(info.summary));
  result.assign_assume_new("description",
                           sourcemeta::core::to_json(info.description));
  result.assign_assume_new("termsOfService",
                           sourcemeta::core::to_json(info.terms_of_service));

  if (info.contact.has_value()) {
    auto contact{sourcemeta::core::JSON::make_object()};
    contact.assign_assume_new("name",
                              sourcemeta::core::to_json(info.contact->name));
    contact.assign_assume_new("url",
                              sourcemeta::core::to_json(info.contact->url));
    contact.assign_assume_new("email",
                              sourcemeta::core::to_json(info.contact->email));
    result.assign_assume_new("contact", std::move(contact));
  } else {
    result.assign_assume_new("contact", sourcemeta::core::JSON{nullptr});
  }

  if (info.license.has_value()) {
    auto license{sourcemeta::core::JSON::make_object()};
    license.assign_assume_new("name",
                              sourcemeta::core::JSON{info.license->name});
    license.assign_assume_new(
        "identifier", sourcemeta::core::to_json(info.license->identifier));
    license.assign_assume_new("url",
                              sourcemeta::core::to_json(info.license->url));
    result.assign_assume_new("license", std::move(license));
  } else {
    result.assign_assume_new("license", sourcemeta::core::JSON{nullptr});
  }

  return result;
}

auto version_string(const sourcemeta::core::OpenAPIVersion version)
    -> sourcemeta::core::JSON::StringView {
  switch (version) {
    // OpenAPI Specification 3.1.1, Section 4.1: "The `major`.`minor` portion
    // of the version string (for example `3.1`) SHALL designate the OAS
    // feature set"
    case sourcemeta::core::OpenAPIVersion::OPENAPI_3_1:
      return "3.1"sv;
    case sourcemeta::core::OpenAPIVersion::OPENAPI_3_2:
      return "3.2"sv;
  }

  std::unreachable();
}

// OpenAPI Specification 3.1.1, Section 4.6 determines a document's base URI
// "in accordance with RFC3986 Section 5.1.2 - 5.1.4", a range that starts at
// 5.1.2 and so leaves out 5.1.1, "Base URI Embedded in Content". A 3.1
// document therefore has no way of declaring its own base, and what remains is
// 5.1.3, "Base URI from the Retrieval URI", which only the caller can supply.
// Section 4.6 says as much: implementations "SHOULD allow users to provide
// documents with their intended retrieval URIs"
auto canonical_base(const std::string_view input)
    -> sourcemeta::core::JSON::String {
  if (input.empty()) {
    return {};
  }

  std::optional<sourcemeta::core::URI> base;
  try {
    base.emplace(input);
  } catch (const sourcemeta::core::URIParseError &) {
    base.reset();
  }

  // RFC 3986 Section 5.2.1: "only the scheme component is required to be
  // present in a base URI". Anything without one cannot resolve a reference
  // RFC 3986 Section 5.2.2 resolves a reference against a base's scheme,
  // authority, path and query, and never against its fragment, so a fragment
  // is no part of what a base is. Keeping one would also put two of them in
  // every location this frame reports
  if (base.has_value() && base.value().scheme().has_value()) {
    base.value().canonicalize();
    const auto result{base.value().recompose_without_fragment()};
    if (result.has_value()) {
      return result.value();
    }
  }

  throw sourcemeta::core::OpenAPIError{
      sourcemeta::core::EMPTY_POINTER,
      "The OpenAPI Description base must be a URI with a scheme"};
}

// A Reference Object, and a Path Item Object that declares a `$ref`, stand in
// for what they lead to. OpenAPI Specification 3.1.1, Section 4.8.9 has `$ref`
// "allow for a referenced definition of this path item" and leaves what a
// sibling field means undefined, so the definition is what the reference leads
// to rather than anything written alongside it
auto follow_aliases(const sourcemeta::core::OpenAPIWalk &walk,
                    const sourcemeta::core::JSON::String &position)
    -> sourcemeta::core::JSON::String {
  return sourcemeta::core::openapi_resolve_position(walk, position);
}

auto identity_of(const sourcemeta::core::OpenAPIWalk &walk,
                 const sourcemeta::core::JSON::String &position)
    -> const std::pair<sourcemeta::core::JSON::String,
                       sourcemeta::core::JSON::String> * {
  return sourcemeta::core::openapi_parameter_identity(walk, position);
}

// The parameters in force where an operation sits. Section 4.8.9 has the ones
// a Path Item Object declares "applicable for all the operations described
// under this path. These parameters can be overridden at the operation level,
// but cannot be removed there", so what the Path Item declares stands unless
// the operation declares one of the same name and location
auto parameters_of(const sourcemeta::core::OpenAPIWalk &walk,
                   const std::vector<sourcemeta::core::JSON::String> &operation,
                   const std::vector<sourcemeta::core::JSON::String> &path_item)
    -> std::vector<sourcemeta::core::JSON::String> {
  std::vector<sourcemeta::core::JSON::String> result;
  result.reserve(operation.size() + path_item.size());
  std::set<
      std::pair<sourcemeta::core::JSON::String, sourcemeta::core::JSON::String>>
      claimed;
  for (const auto &position : operation) {
    const auto *identity{identity_of(walk, position)};
    if (identity != nullptr) {
      claimed.insert(*identity);
    }
  }

  for (const auto &position : path_item) {
    // A Reference Object that was never followed names no parameter, so
    // nothing can be said to override it
    const auto *identity{identity_of(walk, position)};
    if (identity == nullptr || !claimed.contains(*identity)) {
      result.push_back(position);
    }
  }

  result.insert(result.cend(), operation.cbegin(), operation.cend());
  return result;
}

// Which Tag Object each of an operation's tags names. Section 4.3.3 lists this
// among the connections a description makes by name rather than by pointer, and
// it is the one that carries no requirement, since Section 4.8.1 says "not all
// tags that are used by the Operation Object must be declared". So a name
// nothing declares is reported rather than turned down
auto tags_of(const sourcemeta::core::OpenAPIWalk &walk,
             const std::vector<sourcemeta::core::JSON::String> &names)
    -> std::vector<std::optional<sourcemeta::core::JSON::String>> {
  std::vector<std::optional<sourcemeta::core::JSON::String>> result;
  result.reserve(names.size());
  for (const auto &name : names) {
    const auto match{walk.tags.find(name)};
    result.push_back(match == walk.tags.cend() ? std::nullopt
                                               : std::optional{match->second});
  }

  return result;
}

// The servers in force where an operation sits. OpenAPI Specification 3.1.1,
// Section 4.8.10 has an Operation Object's servers override those of "the Path
// Item Object or OpenAPI Object level", and Section 4.8.1 makes an empty array
// there stand for none being given at all, so an empty array carries on up
auto servers_of(const std::vector<sourcemeta::core::JSON::String> &operation,
                const std::vector<sourcemeta::core::JSON::String> &path_item,
                const std::vector<sourcemeta::core::JSON::String> &document)
    -> std::vector<sourcemeta::core::JSON::String> {
  if (!operation.empty()) {
    return operation;
  }

  return path_item.empty() ? document : path_item;
}

// OpenAPI Specification 3.1.1, Section 4.8.12, of a parameter whose location
// is `path`: its "`name` field MUST correspond to a template expression
// occurring within the path field in the Paths Object". This direction holds
// wherever such a parameter is written, including a Path Item Object that
// declares no operation at all
auto check_path_parameters(
    const sourcemeta::core::OpenAPIWalk &walk,
    const std::vector<sourcemeta::core::JSON::StringView> &templates,
    const std::vector<sourcemeta::core::JSON::String> &parameters) -> void {
  for (const auto &position : parameters) {
    const auto *identity{identity_of(walk, position)};
    // A Reference Object that was never followed names no parameter, so
    // nothing can be said about what it corresponds to
    if (identity == nullptr || identity->second != "path") {
      continue;
    }

    if (std::ranges::find(templates, identity->first) == templates.cend()) {
      throw error_at(walk, position,
                     "A path Parameter Object must name a template expression "
                     "of the path it is under");
    }
  }
}

// OpenAPI Specification 3.2.1, Section 4.12, of a parameter whose location
// is `querystring`: it "MUST NOT appear more than once, and MUST NOT appear in
// the same operation (or in the operation's path-item) as any `in: "query"`
// parameters". A Path Item and an Operation are one set where an operation is
// concerned, which is what the parameters in force are
auto check_querystring(
    const sourcemeta::core::OpenAPIWalk &walk,
    const sourcemeta::core::JSON::String &origin,
    const std::vector<sourcemeta::core::JSON::String> &parameters) -> void {
  std::size_t querystrings{0};
  bool query{false};
  for (const auto &position : parameters) {
    const auto *identity{identity_of(walk, position)};
    if (identity == nullptr) {
      continue;
    }

    if (identity->second == "querystring") {
      querystrings += 1;
    } else if (identity->second == "query") {
      query = true;
    }
  }

  if (querystrings > 1) {
    throw error_at(walk, origin,
                   "A querystring Parameter Object must not appear more than "
                   "once among the parameters in force");
  }

  if (querystrings > 0 && query) {
    throw error_at(walk, origin,
                   "A querystring Parameter Object must not appear alongside a "
                   "query Parameter Object");
  }
}

// Section 3.5, the other direction: "Each template expression in the path MUST
// correspond to a path parameter that is included in the Path Item itself
// and/or in each of the Path Item's Operations". So this holds of the
// parameters in force for one operation rather than of either level alone, and
// Section 3.5 excuses an empty Path Item from it, which is why nothing checks
// it until there is an operation to check
auto check_path_templates(
    const sourcemeta::core::OpenAPIWalk &walk,
    const sourcemeta::core::JSON::String &endpoint,
    const std::vector<sourcemeta::core::JSON::StringView> &templates,
    const std::vector<sourcemeta::core::JSON::String> &parameters) -> void {
  std::set<sourcemeta::core::JSON::StringView> named;
  for (const auto &position : parameters) {
    const auto *identity{identity_of(walk, position)};

    // A reference the walk could not follow may be the very parameter a
    // template expression is looking for, and a description we do not hold in
    // full is one we cannot call incomplete. This is the same restraint
    // Section 8.7.1 applies to a Link Object's operation identifier
    if (identity == nullptr) {
      return;
    }

    if (identity->second == "path") {
      named.insert(identity->first);
    }
  }

  for (const auto &expression : templates) {
    if (!named.contains(expression)) {
      throw error_at(walk, endpoint,
                     "A templated path must declare a path parameter for each "
                     "of its template expressions");
    }
  }
}

// Every operation the description exposes, which Section 4.3.3 confines to what
// the entry document reaches: "only the entry document's Paths Object
// contributes URLs to the described API". A Callback Object holds Path Item
// Objects of its own, so what an endpoint reaches may expose further endpoints
auto project(const sourcemeta::core::OpenAPIWalk &walk)
    -> std::vector<sourcemeta::core::OpenAPIOperation> {
  std::vector<sourcemeta::core::OpenAPIOperation> result;
  std::vector<sourcemeta::core::OpenAPIEndpoint> pending{walk.endpoints};
  std::set<sourcemeta::core::JSON::String> seen;
  for (std::size_t index = 0; index < pending.size(); index += 1) {
    const auto kind{pending[index].kind};
    const auto path{pending[index].path};
    const auto endpoint{pending[index].path_item};
    auto position{follow_aliases(walk, endpoint)};

    // A Path Item that leads back to one already exposed the same way exposes
    // nothing further, which is what stops a cycle of them
    sourcemeta::core::JSON::String key{
        sourcemeta::core::openapi_operation_kind_name(kind)};
    key.append("#").append(path).append("#").append(position);
    if (!seen.insert(std::move(key)).second) {
      continue;
    }

    const auto entry{walk.path_items.find(position)};
    if (entry == walk.path_items.cend()) {
      continue;
    }

    // A webhook name and a callback expression are not templated paths, so
    // only what the Paths Object exposes has any templating to correspond to
    const auto templated{kind == sourcemeta::core::OpenAPIOperationKind::Path};
    const auto templates{
        templated ? sourcemeta::core::openapi_brace_expressions(path)
                  : std::vector<sourcemeta::core::JSON::StringView>{}};
    if (templated) {
      check_path_parameters(walk, templates, entry->second.parameters);
    }

    for (const auto &[method, origin] : entry->second.operations) {
      const auto operation{walk.operation_records.find(origin)};
      if (operation == walk.operation_records.cend()) {
        continue;
      }

      auto parameters{parameters_of(walk, operation->second.parameters,
                                    entry->second.parameters)};
      if (templated) {
        check_path_parameters(walk, templates, parameters);
        check_path_templates(walk, endpoint, templates, parameters);
      }

      check_querystring(walk, origin, parameters);

      result.push_back(
          {.kind = kind,
           .path = path,
           .method = method,
           .origin = origin,
           .endpoint = endpoint,
           .servers = servers_of(operation->second.servers,
                                 entry->second.servers, walk.servers),
           // Section 4.8.10: "This definition overrides any declared top-level
           // security. To remove a top-level security declaration, an empty
           // array can be used", which is why declaring none and declaring an
           // empty array are not the same thing here
           .security = operation->second.security.has_value()
                           ? operation->second.security.value()
                           : walk.security.value_or(
                                 std::vector<sourcemeta::core::JSON::String>{}),
           .parameters = std::move(parameters),
           .tags = tags_of(walk, operation->second.tags)});

      for (const auto &callback : operation->second.callbacks) {
        const auto entries{walk.callbacks.find(follow_aliases(walk, callback))};
        if (entries == walk.callbacks.cend()) {
          continue;
        }

        for (const auto &[expression, path_item] : entries->second) {
          pending.push_back(
              {.kind = sourcemeta::core::OpenAPIOperationKind::Callback,
               .path = expression,
               .path_item = path_item});
        }
      }
    }
  }

  return result;
}

auto analyse(const sourcemeta::core::JSON &document,
             sourcemeta::core::JSON::String base)
    -> sourcemeta::core::OpenAPIWalk {
  sourcemeta::core::OpenAPIWalk walk{
      .base = std::move(base),
      .document = &document,
      .operation_ids = {},
      .visited = {},
      .locations = {},
      .references = {},
      .parameters = {},
      .path_items = {},
      .operation_records = {},
      .callbacks = {},
      .endpoints = {},
      .servers = {},
      .security = {},
      .security_schemes = {},
      .tags = {},
      .tag_parents = {},
      .tag_names = {},
      .operation_id_links = {},
      .version = sourcemeta::core::OpenAPIVersion::OPENAPI_3_1,
      .dialect = {},
      .info = {}};
  sourcemeta::core::openapi_check_document(document, walk);
  return walk;
}

} // namespace

namespace sourcemeta::core {

struct OpenAPIFrame::Internal {
  OpenAPIVersion version;
  OpenAPIInfo info;
  // Canonicalising means this no longer borrows from what the caller passed
  JSON::String base;
  bool standalone;
  std::map<JSON::String, OpenAPILocation> locations;
  std::map<JSON::String, OpenAPIReference> references;
  std::vector<OpenAPIOperation> operations;
  // Reading inside a Schema Object is the business of whatever understands
  // JSON Schema, so this is that pass over every Schema Object position at
  // once. It is declared last so that it is destroyed first, as it holds
  // views into the locations above and into the document itself
  SchemaResolver schema_resolver;
  SchemaFrame::Paths schema_paths;
  std::unique_ptr<SchemaFrame> schemas;
};

OpenAPIFrame::OpenAPIFrame(const JSON &document, const SchemaWalker &walker,
                           const SchemaResolver &resolver,
                           const std::string_view default_base,
                           const std::uint64_t max_locations)
    : internal_{std::make_unique<Internal>()} {
  auto walk{analyse(document, canonical_base(default_base))};
  this->internal_->version = walk.version;
  this->internal_->info = walk.info;
  // What the caller passed in is where the entry document was retrieved from,
  // and from 3.2 onwards the document may give itself a URI of its own, which
  // the walk settles and everything it holds is keyed by
  this->internal_->base = std::move(walk.base);
  // A frame stands alone when everything it references is inside it, which is
  // what a caller asks before deciding whether it has the whole description.
  // Which references leave it is what making it whole comes down to, so each
  // one says so of itself rather than only the description as a whole
  bool every_reference_lands{true};
  for (auto &reference : walk.references) {
    reference.second.dangling =
        !walk.locations.contains(reference.second.destination);
    if (reference.second.dangling) {
      every_reference_lands = false;
    }
  }

  // Projecting reads the whole walk, so nothing is taken out of it until after
  this->internal_->operations = project(walk);
  this->internal_->locations = std::move(walk.locations);
  this->internal_->references = std::move(walk.references);

  // Every Schema Object position of the document at once, rather than one
  // pass each, so that a schema referring to another resolves against a frame
  // that holds both. Section 4.8.24.1 scopes `jsonSchemaDialect` to "all
  // Schema Objects contained within an OAS document", and a document has one
  // base, so what those positions have in common is the whole of what this
  // pass needs to be told
  const auto root{this->internal_->locations.find(this->internal_->base)};
  assert(root != this->internal_->locations.cend());

  // What sits inside a Schema Object is JSON Schema's to make sense of, so a
  // reference that names a place in there has the description read one of its
  // own Objects out of a schema. Appendix G of OAS 3.2, and Section 3.2 of
  // 3.1, leave what to do about a place read as two kinds of thing to the
  // implementation and allow saying so, which is what this does. Framing the
  // schemas could not proceed regardless, as it is given each of these
  // positions to frame and they must not sit within one another
  //
  // Locations are keyed by the base with the pointer hung off it, so a place
  // within another has that other one's key as a prefix and follows it here
  std::vector<WeakPointer> enclosing;
  for (const auto &location : this->internal_->locations) {
    if (location.second.type != OpenAPIObjectKind::Schema) {
      continue;
    }

    auto pointer{to_weak_pointer(location.second.pointer)};
    while (!enclosing.empty() && !pointer.starts_with(enclosing.back())) {
      enclosing.pop_back();
    }

    if (!enclosing.empty()) {
      throw OpenAPIError{
          this->internal_->base, location.second.pointer,
          "A Schema Object must not sit within another Schema Object"};
    }

    enclosing.push_back(pointer);
    this->internal_->schema_paths.push_back(std::move(pointer));
  }

  this->internal_->schema_resolver = resolver;
  this->internal_->schemas = std::make_unique<SchemaFrame>(
      SchemaFrame::Mode::References, document, walker, resolver,
      root->second.dialect, "", SchemaFrame::IdentifierMode::Additional,
      this->internal_->schema_paths, this->internal_->base, max_locations);

  // What a Schema Object references is as much a part of the description as
  // what the shell around it does, so a description whose schemas reach for
  // something nobody holds is one that is missing a part of itself just the
  // same
  this->internal_->standalone =
      every_reference_lands && this->internal_->schemas->standalone();

  // Section 4.3.3 has resolving a Link Object `operationId` require "parsing
  // all referenced documents prior to determining an `operationId` to be
  // unresolvable". A description we do not hold in full is one we cannot say
  // that of, so these wait until the whole of it is settled, which counts what
  // the Schema Objects reach for as much as what the shell around them does
  if (this->internal_->standalone) {
    check_operation_id_links(walk, this->internal_->locations);
  }

  // A tag the description declares elsewhere is one this cannot say is
  // missing, for the same reason as the identifiers above
  check_tag_parents(walk, this->internal_->locations,
                    this->internal_->standalone);
}

OpenAPIFrame::~OpenAPIFrame() = default;

auto OpenAPIFrame::version() const noexcept -> OpenAPIVersion {
  return this->internal_->version;
}

auto OpenAPIFrame::info() const noexcept -> const OpenAPIInfo & {
  return this->internal_->info;
}

auto OpenAPIFrame::base() const noexcept -> JSON::StringView {
  return this->internal_->base;
}

auto OpenAPIFrame::standalone() const noexcept -> bool {
  return this->internal_->standalone;
}

auto OpenAPIFrame::schemas() const noexcept -> const SchemaFrame & {
  return *(this->internal_->schemas);
}

auto OpenAPIFrame::to_json() const -> JSON {
  // Read through the accessors rather than the internal state, so that what
  // this reports and what a caller can observe cannot drift apart
  auto result{JSON::make_object()};
  result.assign_assume_new("version", JSON{version_string(this->version())});
  result.assign_assume_new("base", JSON{this->base()});
  result.assign_assume_new("standalone", JSON{this->standalone()});
  result.assign_assume_new("schemas", this->internal_->schemas->to_json(
                                          this->internal_->schema_resolver));
  result.assign_assume_new("info", info_json(this->info()));

  auto locations{JSON::make_object()};
  for (const auto &location : this->internal_->locations) {
    auto entry{JSON::make_object()};
    entry.assign_assume_new("type",
                            JSON{openapi_kind_name(location.second.type)});
    entry.assign_assume_new("pointer",
                            JSON{to_string(location.second.pointer)});
    // The nearest recorded ancestor, which every Object but the root of a
    // document has, since an Object is always recorded before its contents
    entry.assign_assume_new(
        "parent",
        parent_of(this->internal_->locations, location.first, location.second));
    // Only the root of a document and a Schema Object carry one
    if (!location.second.dialect.empty()) {
      entry.assign_assume_new("dialect", JSON{location.second.dialect});
    }

    // A Schema Object position alone carries the other half of what whatever
    // reads inside it needs, and an empty base is as much an answer as any
    // other, so this goes by the kind rather than by the value
    if (location.second.type == OpenAPIObjectKind::Schema) {
      entry.assign_assume_new("base", JSON{location.second.base});
    }

    // Only an Object that declares a `$ref` carries these, which is a
    // Reference Object or a Path Item Object standing in for another
    const auto reference{this->internal_->references.find(location.first)};
    if (reference != this->internal_->references.cend()) {
      entry.assign_assume_new("original", JSON{reference->second.original});
      entry.assign_assume_new("destination",
                              JSON{reference->second.destination});
      entry.assign_assume_new("dangling", JSON{reference->second.dangling});
    }

    locations.assign_assume_new(location.first, std::move(entry));
  }

  result.assign_assume_new("locations", std::move(locations));

  auto operations{JSON::make_array()};
  for (const auto &operation : this->internal_->operations) {
    auto entry{JSON::make_object()};
    entry.assign_assume_new("type",
                            JSON{openapi_operation_kind_name(operation.kind)});
    entry.assign_assume_new("path", JSON{operation.path});
    entry.assign_assume_new("method", JSON{operation.method});
    entry.assign_assume_new("origin", JSON{operation.origin});
    entry.assign_assume_new("endpoint", JSON{operation.endpoint});

    entry.assign_assume_new("tags", sourcemeta::core::to_json(operation.tags));

    entry.assign_assume_new("servers",
                            sourcemeta::core::to_json(operation.servers));

    entry.assign_assume_new("security",
                            sourcemeta::core::to_json(operation.security));

    entry.assign_assume_new("parameters",
                            sourcemeta::core::to_json(operation.parameters));
    operations.push_back(std::move(entry));
  }

  result.assign_assume_new("operations", std::move(operations));
  return result;
}

} // namespace sourcemeta::core
