#include <sourcemeta/core/openapi.h>
#include <sourcemeta/core/text.h>

#include "discriminator.h"
#include "document.h"
#include "helpers.h"
#include "info.h"

#include <algorithm>   // std::ranges::all_of, std::ranges::find
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <map>         // std::map
#include <memory>      // std::make_unique
#include <optional>    // std::optional
#include <set>         // std::set
#include <string_view> // std::string_view
#include <tuple>       // std::tuple
#include <utility>     // std::move, std::pair, std::unreachable
#include <vector>      // std::vector

namespace {
using namespace std::string_view_literals;

// A parent is given as one of the keys a location is held under rather than as
// a bare pointer, so that following it is a lookup in the same map rather than
// a key the reader has to rebuild
auto parent_of(const std::map<sourcemeta::core::JSON::String,
                              sourcemeta::core::OpenAPILocation> &locations,
               const sourcemeta::core::JSON::String &uri,
               const sourcemeta::core::OpenAPILocation &location)
    -> sourcemeta::core::JSON {
  if (location.pointer.empty()) {
    return sourcemeta::core::JSON{nullptr};
  }

  const auto document{sourcemeta::core::openapi_document_uri(uri)};
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

auto error_at(const sourcemeta::core::OpenAPIWalk &walk,
              const sourcemeta::core::JSON::String &location,
              const char *message,
              const sourcemeta::core::JSON::StringView field = {})
    -> sourcemeta::core::OpenAPIError {
  return sourcemeta::core::openapi_error_at(walk.locations, location, message,
                                            field);
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

// A Reference Object, and a Path Item Object that declares a `$ref`, stand in
// for what they lead to. OpenAPI Specification 3.1.1, Section 4.8.9 has `$ref`
// "Allows for a referenced definition of this path item", and leaves undefined
// only what "appears both in the defined object and the referenced object", so
// what the reference leads to is where a place is looked for first. A field
// written beside it that the referenced Path Item Object does not declare is
// an ordinary field of the Object holding it, which the projection reads back
// rather than this
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

// Whether a Parameter Object is one the specification tells a reader to look
// past. Section 4.8.12 names three by their location and their name: "If `in`
// is `"header"` and the `name` field is `"Accept"`, `"Content-Type"` or
// `"Authorization"`, the parameter definition SHALL be ignored". Section
// 4.8.12.1 reads such a name under RFC 7230, which "states header names are
// case insensitive", and 3.2.1 Section 4.12.1 says as much under RFC 9110, so
// which letters it is written with settles nothing
auto ignored_by_the_specification(
    const std::pair<sourcemeta::core::JSON::String,
                    sourcemeta::core::JSON::String> &identity) -> bool {
  if (identity.second != "header") {
    return false;
  }

  auto name{identity.first};
  sourcemeta::core::to_lowercase(name);
  return name == "accept" || name == "content-type" || name == "authorization";
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
    if (identity != nullptr && ignored_by_the_specification(*identity)) {
      continue;
    }

    if (identity == nullptr || !claimed.contains(*identity)) {
      result.push_back(position);
    }
  }

  for (const auto &position : operation) {
    const auto *identity{identity_of(walk, position)};
    if (identity != nullptr && ignored_by_the_specification(*identity)) {
      continue;
    }

    result.push_back(position);
  }

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
// Item Object or OpenAPI Object level", and Section 4.8.9 says as much of a
// Path Item Object's own.
//
// Neither says what an empty array means where it is written. Only Section
// 4.8.1 speaks of one, and only of the array the OpenAPI Object itself holds:
// "If the `servers` field is not provided, or is an empty array, the default
// value would be a Server Object with a url value of `/`". That sentence is no
// authority over the two levels below it,
// so reading an empty array there as nothing given is a choice this makes
// where the specification says nothing, rather than a rule it follows
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
    const std::vector<sourcemeta::core::JSON::String> &parameters,
    const char *message) -> void {
  for (const auto &position : parameters) {
    const auto *identity{identity_of(walk, position)};
    // A Reference Object that was never followed names no parameter, so
    // nothing can be said about what it corresponds to
    if (identity == nullptr || identity->second != "path") {
      continue;
    }

    if (std::ranges::find(templates, identity->first) == templates.cend()) {
      throw error_at(walk, position, message);
    }
  }
}

// Every Path Item Object a position leads through, from the one written down
// to the one the chain ends at. A `$ref` may lead to a Path Item Object that
// declares one of its own, and each of those is a place of the description in
// its own right, so reading only the two ends would pass over whatever the
// middle of a chain writes beside its own reference
auto aliased_path_items(const sourcemeta::core::OpenAPIWalk &walk,
                        const sourcemeta::core::JSON::String &position)
    -> std::vector<const sourcemeta::core::OpenAPIPathItemRecord *> {
  std::vector<const sourcemeta::core::OpenAPIPathItemRecord *> result;
  // Every position walked through is one the caller or the walk already holds,
  // so this runs once per path and keeps no string of its own
  std::set<sourcemeta::core::JSON::StringView> seen;
  const auto *current{&position};
  while (seen.insert(*current).second) {
    const auto record{walk.path_items.find(*current)};
    if (record != walk.path_items.cend()) {
      result.push_back(&record->second);
    }

    const auto alias{walk.references.find(*current)};
    if (alias == walk.references.cend()) {
      break;
    }

    current = &alias->second.destination;
  }

  return result;
}

// The template expressions of every path that exposes a given Path Item
// Object, keyed by where that Path Item sits. The requirement above names the
// Paths Object rather than wherever the parameter happens to be written, so a
// Path Item reached as a webhook or through a callback expression is held to
// what the paths reaching that very Path Item declare, and one no path reaches
// leaves nothing for such a parameter to correspond to
auto exposing_expressions(const sourcemeta::core::OpenAPIWalk &walk)
    -> std::pair<std::map<sourcemeta::core::JSON::String,
                          std::vector<sourcemeta::core::JSON::StringView>>,
                 bool> {
  std::map<sourcemeta::core::JSON::String,
           std::vector<sourcemeta::core::JSON::StringView>>
      result;
  bool whole{true};
  for (const auto &endpoint : walk.endpoints) {
    if (endpoint.kind != sourcemeta::core::OpenAPIOperationKind::Path) {
      continue;
    }

    const auto position{follow_aliases(walk, endpoint.path_item)};
    // A path whose own Path Item Object the walk could not reach may be the
    // very path that exposes another one, so what is gathered here is short of
    // what the description holds and says nothing about a place it does not
    // cover. OpenAPI Specification 3.2.1, Section 4.1.2.1 leaves no room to
    // call a reference unresolvable while a document of the description has
    // gone unread
    if (!walk.path_items.contains(position)) {
      whole = false;
      continue;
    }

    auto &expressions{result[position]};
    for (const auto &expression :
         sourcemeta::core::openapi_brace_expressions(endpoint.path)) {
      expressions.push_back(expression);
    }
  }

  return {std::move(result), whole};
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
auto collect_path_parameter_names(
    const sourcemeta::core::OpenAPIWalk &walk,
    const std::vector<sourcemeta::core::JSON::String> &parameters,
    std::set<sourcemeta::core::JSON::StringView> &names) -> bool {
  for (const auto &position : parameters) {
    const auto *identity{identity_of(walk, position)};

    // A reference the walk could not follow may be the very parameter a
    // template expression is looking for, and a description we do not hold in
    // full is one we cannot call incomplete. This is the same restraint
    // Section 4.3.3 applies to a Link Object's operation identifier, and it
    // stops in the same place: one that ends inside this very document ends
    // where no further reading can supply a parameter, so it is passed over
    // rather than taken as a reason to say nothing
    if (identity == nullptr) {
      if (!sourcemeta::core::openapi_within_document(
              follow_aliases(walk, position), walk.base)) {
        return false;
      }

      continue;
    }

    if (identity->second == "path") {
      names.insert(identity->first);
    }
  }

  return true;
}

auto check_path_templates(
    const sourcemeta::core::OpenAPIWalk &walk,
    const sourcemeta::core::JSON::String &endpoint,
    const std::vector<sourcemeta::core::JSON::StringView> &templates,
    const std::vector<const sourcemeta::core::OpenAPIPathItemRecord *> &chain,
    const std::vector<sourcemeta::core::JSON::String> &parameters) -> void {
  std::set<sourcemeta::core::JSON::StringView> named;
  if (!collect_path_parameter_names(walk, parameters, named)) {
    return;
  }

  // Section 4.8.9 leaves undefined which of two lists a chain writes is in
  // force, so a template expression is answered by a path parameter written at
  // any place the chain leads through. Turning a description down on one
  // reading alone would refuse what another equally licensed reading accepts
  for (const auto *record : chain) {
    if (!collect_path_parameter_names(walk, record->parameters, named)) {
      return;
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

} // namespace

namespace sourcemeta::core {

// Every operation the description exposes, which Section 4.3.3 confines to what
// the entry document reaches: "only the entry document's Paths Object
// contributes URLs to the described API". A Callback Object holds Path Item
// Objects of its own, so what an endpoint reaches may expose further endpoints
auto openapi_project(const OpenAPIWalk &walk) -> std::vector<OpenAPIOperation> {
  std::vector<OpenAPIOperation> result;
  std::vector<OpenAPIEndpoint> pending{walk.endpoints};
  const auto expressions{exposing_expressions(walk)};
  // What tells one exposure from another, held as the four things it is
  // rather than as one string spelling them out. A position carries a `#` of
  // its own, so any character picked to join them is a character one of them
  // may hold, and the spelling would not tell every pair of exposures apart
  std::set<std::tuple<OpenAPIOperationKind, JSON::String, JSON::String,
                      std::optional<JSON::String>>>
      seen;
  for (std::size_t index = 0; index < pending.size(); index += 1) {
    const auto kind{pending[index].kind};
    const auto path{pending[index].path};
    const auto endpoint{pending[index].path_item};
    const auto parent{pending[index].parent};
    auto position{follow_aliases(walk, endpoint)};

    // A Path Item that leads back to one already exposed the same way exposes
    // nothing further, which is what stops a cycle of them. Section 4.8.10
    // hangs a Callback Object off "the parent operation", so one that two of
    // them reach is reached twice rather than once and the Object it hangs off
    // is part of what tells the two apart
    if (!seen.emplace(kind, path, position, parent).second) {
      continue;
    }

    // Section 4.8.9 leaves undefined only what "appears both in the defined
    // object and the referenced object", so a field written beside a `$ref`
    // that the referenced Path Item Object does not declare is an ordinary
    // field of the Path Item Object holding it, and Section 3.5 counts a path
    // parameter "included in the Path Item itself" wherever it is written. So
    // what the reference leads to answers first, and what sits beside it
    // answers for whatever that leaves unsaid
    const auto chain{aliased_path_items(walk, endpoint)};
    if (chain.empty()) {
      continue;
    }

    // Whether what the chain ends at is settled by the document in hand. It is
    // settled when that place is a Path Item Object this holds, and equally
    // when the chain ends on a pointer into this very document that leads
    // nowhere, since no further reading can rescue one of those. 3.2.1 Section
    // 4.1.2.1 asks for every document to be parsed before a reference is
    // called unresolvable, which leaves only a chain ending in a document this
    // has not read unsettled
    const auto settled{walk.path_items.contains(position) ||
                       openapi_within_document(position, walk.base)};

    // The last place the chain leads through that this one holds answers
    // first, which is what the chain ends at whenever that landed, and each
    // place nearer to it answers before the one that names it. Section 4.8.9
    // leaves that order undefined between any two of them and so free to pick
    const auto *parameters_of_path_item{&chain.back()->parameters};
    const auto *servers_of_path_item{&chain.back()->servers};
    auto methods{chain.back()->operations};
    for (const auto *record : std::ranges::reverse_view{chain}) {
      if (parameters_of_path_item->empty()) {
        parameters_of_path_item = &record->parameters;
      }

      if (servers_of_path_item->empty()) {
        servers_of_path_item = &record->servers;
      }

      for (const auto &method : record->operations) {
        if (std::ranges::none_of(methods, [&method](const auto &known) -> bool {
              return known.first == method.first;
            })) {
          methods.push_back(method);
        }
      }
    }

    const auto &path_item_parameters{*parameters_of_path_item};
    const auto &path_item_servers{*servers_of_path_item};

    // A webhook name and a callback expression are not templated paths, so
    // only what the Paths Object exposes has any templating of its own
    const auto templated{kind == OpenAPIOperationKind::Path};
    const auto own_templates{templated ? openapi_brace_expressions(path)
                                       : std::vector<JSON::StringView>{}};
    // Section 4.8.12 asks a path Parameter Object to name "a template
    // expression occurring within the path field in the Paths Object" rather
    // than one of whichever path it happens to sit under, and one Path Item
    // Object may be reached from several of them. So every path exposing this
    // one answers, which is the reading a webhook and a callback already went
    // by, and a Path Item Object no path reached falls back to the path being
    // projected
    const auto exposed{expressions.first.find(position)};
    const auto &templates{
        exposed == expressions.first.cend() ? own_templates : exposed->second};
    // A path answers for its own braces whatever else went unread, but the set
    // gathered from every path exposing one Path Item Object is short by
    // whatever a path this does not hold would have added to it, so only the
    // first of those two is worth reading against on its own
    const auto own_only{templated && exposed == expressions.first.cend()};
    const auto *const message{
        "A path Parameter Object must name a template expression of a path "
        "that exposes it"};
    // What no path exposes is only known to be exposed by none once every path
    // has been read, which a description held short of its documents leaves
    // unsettled
    // Section 4.8.12 binds every Parameter Object written down rather than
    // whichever list the fold above carries forward, so each place the chain
    // leads through answers for the ones it declares itself
    // A path answers for its own braces whatever else is unread, but every
    // other kind of endpoint is held to the expressions of the paths reaching
    // the Path Item Object the chain ends at, and a chain that ends somewhere
    // this does not hold is one whose end an unread document still decides
    if ((own_only || expressions.second) && settled) {
      for (const auto *record : chain) {
        check_path_parameters(walk, templates, record->parameters, message);
        // An Operation Object that a method of the same name nearer the end of
        // the chain outranks is written down all the same, and Section 4.8.12
        // binds a Parameter Object wherever it is written
        for (const auto &declared : record->operations) {
          const auto operation{walk.operation_records.find(declared.second)};
          if (operation != walk.operation_records.cend()) {
            check_path_parameters(walk, templates, operation->second.parameters,
                                  message);
          }
        }
      }
    }

    for (const auto &[method, origin] : methods) {
      const auto operation{walk.operation_records.find(origin)};
      if (operation == walk.operation_records.cend()) {
        continue;
      }

      auto parameters{parameters_of(walk, operation->second.parameters,
                                    path_item_parameters)};
      // The two loops above already answer for every Parameter Object written
      // at any place the chain leads through, and what is in force here is
      // drawn from those same lists, so this holds nothing new. It abstains
      // on the same terms all the same, as the expressions it would be read
      // against are the ones an unread document settles
      if ((own_only || expressions.second) && settled) {
        check_path_parameters(walk, templates, parameters, message);
      }
      // This one turns on an expression having no parameter anywhere, and a
      // chain that ends somewhere this does not hold may well end at the Path
      // Item Object declaring it, so there is nothing to conclude yet
      if (templated && settled) {
        check_path_templates(walk, endpoint, own_templates, chain,
                             operation->second.parameters);
      }

      // Which of the chain's lists is in force decides whether these two ever
      // meet, and that is not settled while the chain ends somewhere unread
      if (settled) {
        check_querystring(walk, origin, parameters);
      }

      result.push_back(
          {.kind = kind,
           .path = path,
           .method = method,
           .origin = origin,
           .endpoint = endpoint,
           .parent = parent,
           .servers = servers_of(operation->second.servers, path_item_servers,
                                 walk.servers),
           // Section 4.8.10: "This definition overrides any declared top-level
           // security. To remove a top-level security declaration, an empty
           // array can be used", which is why declaring none and declaring an
           // empty array are not the same thing here
           .security =
               operation->second.security.has_value()
                   ? operation->second.security.value()
                   : walk.security.value_or(std::vector<JSON::String>{}),
           .parameters = std::move(parameters),
           .tags = tags_of(walk, operation->second.tags)});

      for (const auto &callback : operation->second.callbacks) {
        const auto entries{walk.callbacks.find(follow_aliases(walk, callback))};
        if (entries == walk.callbacks.cend()) {
          continue;
        }

        for (const auto &[expression, path_item] : entries->second) {
          pending.push_back({.kind = OpenAPIOperationKind::Callback,
                             .path = expression,
                             .path_item = path_item,
                             .parent = origin});
        }
      }
    }
  }

  return result;
}

struct OpenAPIFrame::Internal {
  OpenAPIVersion version;
  OpenAPIInfo info;
  // Canonicalising means this no longer borrows from what the caller passed
  JSON::String base;
  bool standalone;
  std::map<JSON::String, OpenAPILocation> locations;
  std::map<JSON::String, OpenAPIReference> references;
  std::vector<OpenAPIOperation> operations;
  // What a Discriminator Object names by URI, which is a reference the schemas
  // hold rather than one the shell around them does
  std::vector<OpenAPIDiscriminator> discriminators;
  std::map<JSON::String, OpenAPIReference> security_references;
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
  auto walk{sourcemeta::core::openapi_analyse(
      document, sourcemeta::core::openapi_canonical_base(default_base),
      max_locations)};
  this->internal_->version = walk.version;
  this->internal_->info = walk.info;
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

  // And so does a Security Requirement Object that names a scheme by the URI
  // of one, which 3.2 admits alongside the name of a component. Naming one
  // that this document does not hold leaves the description no more whole than
  // any other reference out of it would
  for (auto &reference : walk.security_references) {
    reference.second.dangling =
        !walk.locations.contains(reference.second.destination);
    if (reference.second.dangling) {
      every_reference_lands = false;
    }
  }

  // Projecting reads the whole walk, the base included, so nothing is taken
  // out of it until after
  this->internal_->operations = openapi_project(walk);
  // What the caller passed in is where the entry document was retrieved from,
  // and from 3.2 onwards the document may give itself a URI of its own, which
  // the walk settles and everything it holds is keyed by
  this->internal_->base = std::move(walk.base);
  const auto walk_locations{walk.locations.size()};
  this->internal_->locations = std::move(walk.locations);
  this->internal_->references = std::move(walk.references);
  this->internal_->security_references = std::move(walk.security_references);

  // Every Schema Object position of the document at once, rather than one
  // pass each, so that a schema referring to another resolves against a frame
  // that holds both. Section 4.8.24.5 scopes `jsonSchemaDialect` to "all
  // Schema Objects contained within an OAS document", and a document has one
  // base, so what those positions have in common is the whole of what this
  // pass needs to be told
  const auto root{this->internal_->locations.find(this->internal_->base)};
  assert(root != this->internal_->locations.cend());

  // What sits inside a Schema Object is JSON Schema's to make sense of, so a
  // reference that names a place in there has the description read one of its
  // own Objects out of a schema. Appendix G of OAS 3.2, and Section 4.3.2
  // of 3.1, leave what to do about a place read as two kinds of thing to the
  // implementation and allow saying so, which is what this does. Framing the
  // schemas could not proceed regardless, as it is given each of these
  // positions to frame and they must not sit within one another
  //
  // Locations are keyed by the base with the pointer hung off it, so a place
  // within another has that other one's key as a prefix and follows it here
  for (const auto &location : this->internal_->locations) {
    if (location.second.type == OpenAPIObjectKind::Schema) {
      this->internal_->schema_paths.push_back(
          to_weak_pointer(location.second.pointer));
    }
  }

  this->internal_->schema_resolver = resolver;
  // What the shell of a description goes by and what its schemas go by are
  // places of the one description, so they spend from the one allowance. The
  // walk above has already spent its share, and it never spends more than it
  // was handed
  assert(walk_locations <= max_locations);
  try {
    this->internal_->schemas = std::make_unique<SchemaFrame>(
        SchemaFrame::Mode::References, document, walker, resolver,
        root->second.dialect, "", SchemaFrame::IdentifierMode::Additional,
        this->internal_->schema_paths, this->internal_->base,
        max_locations - walk_locations);
  } catch (const SchemaFrameLimitError &) {
    // Framing the schemas was handed what was left rather than the whole, so
    // the allowance it reports is not the one the caller set
    throw OpenAPIFrameLimitError{max_locations};
  }

  // OpenAPI Specification 3.1.1, Section 4.3 lists the URI form of a
  // Discriminator Object `mapping` among the fields that connect the documents
  // of a description, and Appendix G of 3.2 keeps it among the connections a
  // description makes. It is the one of them that a schema frame does not
  // read, as the keyword it sits under belongs to the dialect this
  // specification publishes rather than to JSON Schema
  this->internal_->discriminators =
      openapi_discriminators(document, *(this->internal_->schemas),
                             this->internal_->base, walker, resolver);
  const auto every_mapping_lands{
      std::ranges::all_of(this->internal_->discriminators,
                          [this](const auto &discriminator) -> bool {
                            return openapi_discriminator_lands(
                                *(this->internal_->schemas), discriminator);
                          })};

  // What a Schema Object references is as much a part of the description as
  // what the shell around it does, so a description whose schemas reach for
  // something nobody holds is one that is missing a part of itself just the
  // same
  this->internal_->standalone = every_reference_lands && every_mapping_lands &&
                                this->internal_->schemas->standalone();

  // Section 4.3.3 has resolving a Link Object `operationId` require "parsing
  // all referenced documents prior to determining an `operationId` to be
  // unresolvable". A description we do not hold in full is one we cannot say
  // that of, so these wait until the whole of it is settled, which counts what
  // the Schema Objects reach for as much as what the shell around them does
  if (this->internal_->standalone) {
    sourcemeta::core::openapi_check_operation_id_links(
        walk, this->internal_->locations);
  }

  // A tag the description declares elsewhere is one this cannot say is
  // missing, for the same reason as the identifiers above
  sourcemeta::core::openapi_check_tag_parents(walk, this->internal_->locations,
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

    // Only an operation that a Callback Object exposes has one of these, which
    // is the Operation Object that Callback Object hangs off
    if (operation.parent.has_value()) {
      entry.assign_assume_new("parent", JSON{operation.parent.value()});
    }

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

  // Only a description whose schemas name one carries these, so a description
  // that names none reports nothing rather than an empty list
  if (!this->internal_->discriminators.empty()) {
    auto discriminators{JSON::make_array()};
    for (const auto &discriminator : this->internal_->discriminators) {
      auto entry{JSON::make_object()};
      entry.assign_assume_new("pointer", JSON{to_string(discriminator.origin)});
      entry.assign_assume_new("destination", JSON{discriminator.destination});
      entry.assign_assume_new("scope", JSON{discriminator.scope});
      entry.assign_assume_new("dangling",
                              JSON{!openapi_discriminator_lands(
                                  *(this->internal_->schemas), discriminator)});
      discriminators.push_back(std::move(entry));
    }

    result.assign_assume_new("discriminators", std::move(discriminators));
  }

  if (!this->internal_->security_references.empty()) {
    auto references{JSON::make_array()};
    for (const auto &reference : this->internal_->security_references) {
      auto entry{JSON::make_object()};
      entry.assign_assume_new("pointer",
                              JSON{to_string(reference.second.origin)});
      entry.assign_assume_new("original", JSON{reference.second.original});
      entry.assign_assume_new("destination",
                              JSON{reference.second.destination});
      entry.assign_assume_new("dangling", JSON{reference.second.dangling});
      references.push_back(std::move(entry));
    }

    result.assign_assume_new("securityReferences", std::move(references));
  }

  return result;
}

} // namespace sourcemeta::core
