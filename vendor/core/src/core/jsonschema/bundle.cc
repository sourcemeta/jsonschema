#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/uri.h>

#include "helpers.h"

#include <algorithm>     // std::ranges::any_of, std::ranges::none_of
#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint64_t
#include <functional>    // std::cref
#include <map>           // std::map
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

auto is_skippable_metaschema_reference(const SchemaBundleOptions::Mode mode,
                                       const WeakPointer &pointer,
                                       const std::string &destination) -> bool {
  assert(!pointer.empty());
  assert(pointer.back().is_property());
  if (pointer.back().to_property() != "$schema") {
    return false;
  }

  return mode == SchemaBundleOptions::Mode::References ||
         schema_is_official(destination);
}

// RFC 3986, section 4.4 calls a reference that leads back to the document
// holding it a same-document reference, whose "most frequent examples [...] are
// relative references that are empty or include only the number sign ('#')
// separator followed by a fragment identifier". Those stay as spelled, as they
// need no base URI beyond the one already in effect. Any other relative
// reference names another document, so bundling restates it as the URI it
// resolves to, which is what lets it keep naming its target once the bundled
// document travels somewhere else
//
// A dynamic reference needs no exception here. JSON Schema 2020-12, section
// 8.2.3.2 has it "resolved against the current URI base" like any other before
// the dynamic scope is consulted, so the absolute form of that resolution is
// what the document should spell, and framing only reports the anchor it ends
// up at in place of that resolution when the two name the same place anyway.
// JSON Schema 2019-09, section 8.2.4.2.1 defines the behavior of
// `$recursiveRef` "only for the value `#`", which the rule below already spares
auto spells_another_document_relatively(const SchemaFrame::Reference &reference)
    -> bool {
  if (reference.original == reference.destination) {
    return false;
  }

  const URI original{reference.original};
  return original.is_relative() && !original.is_fragment_only() &&
         !original.empty();
}

// The absolute form of a reference whose target answers to an identifier other
// than the URI it was resolved by
auto rebase_reference(const JSON::String &base,
                      const std::optional<std::string_view> &fragment)
    -> JSON::String {
  URI result{base};
  if (fragment.has_value()) {
    result.fragment(fragment.value());
  }

  return result.recompose();
}

// The URI that a relative reference spelled against a base resolves to, or
// nothing where the spelling already names its target on its own, where the
// base cannot settle what it names, or where what is spelled is no URI
// reference at all. An absolute base is what the last of those asks for, as
// resolving against a relative one settles nothing and the module merges what
// it can rather than refusing
//
// Resolution is the whole of it, with no normalisation after. RFC 3986,
// section 6.2.3 holds that "two URIs that differ only by the suffix \"#\" are
// considered different regardless of the scheme", and the normalisation that
// drops such a fragment answers to section 8.2.1 of the JSON Schema
// specification, which asks it of an identifier rather than of every URI a
// schema writes. Nothing is lost by leaving it out, as section 5.2.2 removes
// dot segments as part of resolving rather than as normalisation
auto restated_uri(const std::string_view uri, const std::string_view base)
    -> std::optional<JSON::String> {
  try {
    URI result{uri};
    if (!result.is_relative()) {
      return std::nullopt;
    }

    const URI anchor{base};
    if (!anchor.is_absolute()) {
      return std::nullopt;
    }

    result.resolve_from(anchor);
    return result.recompose();
  } catch (const URIParseError &) {
    return std::nullopt;
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.24 puts an External Documentation
// Object on a Schema Object, and Section 4.8.11 makes its `url` "The URI for
// the target documentation". Section 4.6 has every URI a description writes
// resolve against the base in effect where it sits, which is the same footing
// a reference stands on, so this one is spelled out exactly as one, wherever
// it sits and whatever base it answers to. A document that bundling has
// gathered is meant to travel away from the bases its parts were read
// against, and spelling every relative URI out is what lets each of them go
// on naming what it named. Leaving one of two identical resources behind
// would also make them differ over nothing
//
// What that base is comes from the framing rather than from this field. It is
// the nearest enclosing identifier wherever the dialect reserves a keyword for
// one, and otherwise the identity the document itself was read under, which is
// the only base a revision that reserves no such keyword can offer
//
// Earlier revisions read the field the other way around. OpenAPI
// Specification 3.0.3 calls it a URL and has every relative URL resolve
// "using the URLs defined in the Server Object as a Base URI", carving out
// nothing but a reference, and 3.1.0 says as much of a URL too. Only 3.0.4
// and 3.1.1 put the field back on the footing of every other URI, the former
// leaving the choice "implementation_defined" while naming this reading
// RECOMMENDED. One rule serves every revision here, as a Schema Object
// carries no Server Object of its own and nothing at this level can reach
// one, so the older reading is not a choice this has to make. It becomes one
// where a whole description is bundled, which does have those URLs in view
//
// RFC 3986, section 4.4 spares an empty or fragment-only reference from this
// elsewhere, a same-document reference needing no base beyond the one already
// in effect. That reasoning does not reach here, as the document one of those
// leads back to is the one the schema was read from rather than whichever one
// it ends up in
auto restate_external_documentation(JSON &subschema, const SchemaFrame &frame,
                                    const SchemaWalker &walker,
                                    const SchemaResolver &resolver) -> void {
  // Which keywords a dialect is made of is settled per schema resource rather
  // than per subschema, and the locations of one resource come together, so
  // the answer for the last dialect seen carries to the next subschema. An
  // empty dialect names nothing this could ask about, which is what the
  // initial state already stands for
  std::string_view current_dialect;
  bool dialect_documents{false};

  frame.for_each_subschema([&subschema, &frame, &walker, &resolver,
                            &current_dialect,
                            &dialect_documents](const auto &location) -> void {
    if (location.dialect != current_dialect) {
      current_dialect = location.dialect;
      dialect_documents = dialect_defines(
          walker, frame.vocabularies(location, resolver), "externalDocs");
    }

    // A dialect that gives the keyword no meaning leaves it holding whatever
    // the schema put there, which moving cannot break
    if (!dialect_documents) {
      return;
    }

    auto &schema{get(subschema, location.pointer)};
    if (!schema.is_object()) {
      return;
    }

    auto *documentation{
        schema.try_at("externalDocs"sv, JSONSCHEMA_HASH_EXTERNAL_DOCS)};
    if (documentation == nullptr || !documentation->is_object()) {
      return;
    }

    auto *written{documentation->try_at("url"sv, JSONSCHEMA_HASH_URL)};
    if (written == nullptr || !written->is_string()) {
      return;
    }

    // What a description spells there is its own to answer for, as bundling is
    // not where a Schema Object is checked
    auto restated{restated_uri(written->to_string(), location.base)};
    if (restated.has_value()) {
      *written = JSON{std::move(restated).value()};
    }
  });
}

// The dialect a schema declares, falling back to the given default
auto declared_dialect(const JSON &schema,
                      const std::string_view default_dialect)
    -> std::string_view {
  if (!schema.is_object()) {
    return default_dialect;
  }

  const auto *dialect{schema.try_at("$schema")};
  return (dialect != nullptr && dialect->is_string()) ? dialect->to_string()
                                                      : default_dialect;
}

// Every frame that bundling constructs spends from the same limit, as how
// many frames it ends up needing is a function of what the resolver hands
// back rather than of the schema the caller passed in. A frame that ran past
// what was left of the limit threw rather than returned, so what it holds is
// always within it
auto charge(std::uint64_t &remaining, const SchemaFrame &frame) -> void {
  assert(frame.location_count() <= remaining);
  remaining -= frame.location_count();
}

// A boolean schema carries no keyword at all, so it has nowhere to hold the
// identifier that JSON Schema 2020-12, section 9.3.1 asks of every embedded
// resource: "Each embedded JSON Schema Resource MUST identify itself with a
// URI using the `$id` keyword". Section 4.3.2 names the object that says what
// each of the two says, "true: Always passes validation, as if the empty
// schema `{}`" and "false: Always fails validation, as if the schema `{ "not":
// {} }`", and restating a remote that way is what lets it carry the name,
// which in turn leaves every reference that found it spelled as it was
// written, as the same section goes on to require
auto restate_boolean(JSON &schema) -> void {
  assert(schema.is_boolean());
  const auto permissive{schema.to_boolean()};
  schema.into_object();
  if (!permissive) {
    schema.assign("not", JSON::make_object());
  }
}

// Only a dialect that gives `not` a meaning can say what `false` says, which
// the section above notes by placing the keyword "in the subschema application
// vocabulary". Writing it under a dialect that draws on no such vocabulary
// would leave an annotation that says nothing, turning the schema that refuses
// everything into the one that accepts it, so such a remote is pointed at
// where it lands instead
auto boolean_can_be_restated(const SchemaWalker &walker,
                             const SchemaVocabularies &vocabularies,
                             const JSON &schema) -> bool {
  assert(schema.is_boolean());
  return schema.to_boolean() || dialect_defines(walker, vocabularies, "not"sv);
}

// A schema that cannot name itself answers to nothing once it is embedded, so
// every reference to one has to spell out where it went. Neither half is known
// where the reference is found: the embedding settles the position, and
// elevation can move a schema again afterwards. So the walk only records which
// identifiers went in unnamed and where each landed, and the references
// themselves are settled against the finished document
struct PositionalReferences {
  // Recorded while walking, as that is where the dialect of a remote is known
  std::unordered_set<JSON::String> unnamed;
  // Recorded while embedding, as that is where the position is settled. Only
  // the unnamed ones are kept, since a schema that carries its own name is
  // reached by that name wherever it ends up
  std::unordered_map<JSON::String, Pointer> landings;
};

// Reading the finished document is what makes this independent of how the
// bundling got there. A reference to an unnamed schema is left unresolved by
// construction, as the identity it named is gone, so framing hands back every
// one of them wherever they ended up
// A dialect that reserves no property name for declaring itself leaves an
// embedded schema no way to say what it is, and what it inherits from wherever
// it lands would be wrong. Marking every subschema is what carries it, since
// the keyword that names a dialect speaks for the one subschema it sits on.
// Only the frame knows where those subschemas are, and taking its word for it
// is what keeps a value that merely looks like a schema from being marked
auto mark_dialect_of_every_subschema(JSON &schema, const SchemaWalker &walker,
                                     const SchemaResolver &resolver,
                                     const std::string_view dialect,
                                     std::uint64_t &remaining) -> void {
  const SchemaFrame frame{SchemaFrame::Mode::Locations,
                          schema,
                          walker,
                          resolver,
                          dialect,
                          "",
                          SchemaFrame::IdentifierMode::Additional,
                          {EMPTY_WEAK_POINTER},
                          "",
                          remaining};
  charge(remaining, frame);

  std::vector<Pointer> targets;
  frame.for_each_location([&targets, dialect](const auto, const auto &,
                                              const auto &location) -> void {
    if (location.type != SchemaFrame::LocationType::Resource &&
        location.type != SchemaFrame::LocationType::Subschema) {
      return;
    }

    // Marking stands in for a declaration that this dialect has no keyword
    // for, so it has nothing to say where a subschema already reads as
    // something else. Going by the dialect in force rather than the one asked
    // for is what leaves such a subschema, and everything under it, as
    // whoever wrote it meant
    if (location.dialect != dialect) {
      return;
    }

    targets.push_back(to_pointer(location.pointer));
  });

  for (const auto &target : targets) {
    auto &subschema{get(schema, target)};
    // A boolean carries no keyword whose meaning could have differed, so
    // leaving it to inherit costs nothing
    if (subschema.is_object()) {
      subschema.assign(JSON::String{DIALECT_OVERRIDE_KEYWORD},
                       JSON{JSON::String{dialect}});
    }
  }
}

// The key this assigns is the only record of where the schema landed, which a
// reference that names a place rather than an identifier has to be told
auto canonical_uri(const std::string_view uri) -> JSON::String {
  try {
    return URI::canonicalize(uri);
  } catch (const URIParseError &) {
    return JSON::String{uri};
  }
}

// Every meta-schema that anything within this schema pins. A schema that
// looks for one of these has to go on finding it, so collecting only what the
// top of the document declares would miss a resource further in that pins its
// own
auto collect_pinned_dialects(const JSON &schema,
                             std::unordered_set<JSON::String> &result) -> void {
  if (schema.is_object()) {
    const auto *declared{schema.try_at("$schema"sv, JSONSCHEMA_HASH_SCHEMA)};
    if ((declared != nullptr) && declared->is_string()) {
      result.insert(canonical_uri(declared->to_string()));
    }

    for (const auto &entry : schema.as_object()) {
      collect_pinned_dialects(entry.second, result);
    }
  } else if (schema.is_array()) {
    for (const auto &item : schema.as_array()) {
      collect_pinned_dialects(item, result);
    }
  }
}

auto embed_schema(JSON &root, const Pointer &container,
                  const std::string_view identifier, JSON &&target,
                  const SchemaBundleOptions::Callback &callback)
    -> JSON::String {
  auto *current{&root};
  for (const auto &token : container) {
    if (token.is_property()) {
      current->assign_if_missing(token.to_property(), JSON::make_object());
      current = &current->at(token.to_property());
    } else {
      // Neither of these can be left to a check that only runs in debug: the
      // step above makes a missing place hold names, so reading a position out
      // of it in a release build reads storage that was never written
      //
      // A place that holds names cannot be stepped into by position at all,
      // which is a different thing from a position that reaches past what a
      // place holds, so the two are told apart the way the step below tells
      // the kind of a place apart
      if (!current->is_array()) {
        throw SchemaContainerError(container,
                                   "Could not bundle to a container that is "
                                   "not an array");
      }

      if (current->size() <= token.to_index()) {
        throw SchemaContainerError(
            container, "Could not bundle to a container that does not exist");
      }

      current = &current->at(token.to_index());
    }
  }

  if (!current->is_object()) {
    throw SchemaContainerError(container,
                               "Could not bundle to a container that is not "
                               "an object");
  }

  std::string key{identifier};
  // Ensure we get a definitions entry that does not exist
  while (current->defines(key)) {
    key += "/x";
  }

  current->assign(key, std::move(target));

  if (callback) {
    auto location{to_weak_pointer(container)};
    location.push_back(std::cref(key));
    callback(identifier, location);
  }

  return key;
}

// Where a reference is written decides what it resolves against, which is the
// resource holding it rather than the one around that. A schema that declares
// an identifier of its own is that resource, so a reference sitting on it
// reads under its name, and only a schema that declares none looks outwards
// for the closest one. Taking what encloses a resource would name a scope the
// reference never reads under. A place that names itself is held under more
// than one entry, so which of them answers this has to be asked for by type
auto enclosing_resource(const SchemaFrame &frame, const WeakPointer &schema)
    -> Pointer {
  if (frame.traverse(schema, SchemaFrame::LocationType::Resource).has_value()) {
    return to_pointer(schema);
  }

  const auto location{frame.traverse(schema)};
  assert(location.has_value());
  const auto &parent{location.value().get().parent};
  return parent.has_value() ? to_pointer(parent.value()) : Pointer{};
}

// A resource that carries its own name cannot name a place of the document it
// sits in, so what it reaches instead is a copy of its own
struct PositionalCopy {
  Pointer host;
  JSON::String keyword;
  JSON::String identifier;
  std::optional<Pointer> tail;
  Pointer reference;
  // What the copy is read under where it came from, which the place it goes
  // to may not say for it
  JSON::String dialect;
  bool carries_its_dialect;
};

auto settle_positional_references(
    JSON &schema, const SchemaWalker &walker, const SchemaResolver &resolver,
    std::string_view default_dialect, std::string_view default_id,
    const SchemaFrame::Paths &paths, std::string_view default_base,
    const PositionalReferences &positional, std::uint64_t &remaining,
    const SchemaBundleOptions::Callback &callback) -> void {
  if (positional.unnamed.empty()) {
    return;
  }

  // What the caller asked to be framed does not have to reach the container,
  // as a wrapper format keeps one outside every schema it frames. So each
  // landing is framed in its own right, which is also what the walk does when
  // it recurses into a remote
  SchemaFrame::Paths settling{paths};
  for (const auto &landing : positional.landings) {
    const auto pointer{to_weak_pointer(landing.second)};
    // Framing takes paths that do not sit inside one another, so a landing
    // the caller already covers is left to the path that covers it
    if (std::ranges::none_of(paths, [&pointer](const auto &path) -> bool {
          return pointer.starts_with(path) || path.starts_with(pointer);
        })) {
      settling.push_back(pointer);
    }
  }

  const SchemaFrame frame{SchemaFrame::Mode::References,
                          schema,
                          walker,
                          resolver,
                          default_dialect,
                          default_id,
                          SchemaFrame::IdentifierMode::Additional,
                          settling,
                          default_base,
                          remaining};
  charge(remaining, frame);

  std::vector<std::pair<Pointer, JSON::String>> rewrites;
  std::vector<PositionalCopy> copies;
  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) -> void {
    const JSON::String base{reference.base};
    const auto landing{positional.landings.find(base)};
    if (landing == positional.landings.cend()) {
      return;
    }

    std::optional<Pointer> tail;
    if (reference.fragment.has_value() && !reference.fragment.value().empty()) {
      tail = fragment_to_pointer(URI{reference.destination});
      if (!tail.has_value()) {
        // Whatever the fragment names, it is not a place of the schema, and a
        // dialect that reserves no keyword for a name reserves none for an
        // anchor either, so there is nothing for it to have named
        throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                   "Could not resolve schema reference");
      }
    }

    auto within_document{landing->second};
    if (tail.has_value()) {
      within_document.push_back(Pointer{tail.value()});
    }

    if (try_get(schema, within_document) == nullptr) {
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    // A pointer names a place of whichever resource is in force where it is
    // written, so what names the landing is where it sits within the resource
    // that holds it rather than within the document
    const auto landed{frame.traverse(to_weak_pointer(landing->second))};
    assert(landed.has_value());
    const auto &landed_location{landed.value().get()};
    auto target{
        landed_location.parent.has_value()
            ? to_pointer(landed_location.pointer)
                  .resolve_from(to_pointer(landed_location.parent.value()))
            : to_pointer(landed_location.pointer)};
    if (tail.has_value()) {
      target.push_back(Pointer{tail.value()});
    }

    // Written on its own that pointer says the same thing only from a scope
    // that resolves against the same base. From anywhere else it takes the
    // name of the resource in front of it, and where that resource answers to
    // no name there is nothing to put there
    const auto enclosing{frame.traverse(pointer.initial())};
    auto value{to_uri(target)};
    if (enclosing.has_value() &&
        enclosing.value().get().base != landed_location.base) {
      if (landed_location.base.empty()) {
        // The landing can only be named on its own from the scope the document
        // itself answers to, and a resource that carries its own name is not
        // that scope. What such a resource needs is its own copy, in whichever
        // location its own dialect reserves for one, which is then a place of
        // that resource and travels with it
        const auto host{enclosing_resource(frame, pointer.initial())};
        const auto host_location{frame.traverse(to_weak_pointer(host))};
        assert(host_location.has_value());
        const auto keyword{definitions_keyword(
            walker, frame.vocabularies(host_location.value().get(), resolver))};
        if (keyword.empty()) {
          throw SchemaReferenceError(
              reference.destination, to_pointer(pointer),
              "Could not reach a schema that this dialect embeds without an "
              "identifier from a resource that reserves nowhere to put one");
        }

        copies.push_back(
            {.host = host,
             .keyword = JSON::String{keyword},
             .identifier = base,
             .tail = tail,
             .reference = to_pointer(pointer),
             .dialect = JSON::String{landed_location.dialect},
             .carries_its_dialect = host_location.value().get().dialect !=
                                    landed_location.dialect});
        return;
      }

      URI absolute{JSON::String{landed_location.base}};
      absolute.fragment(value.fragment().value());
      value = std::move(absolute);
    }

    rewrites.emplace_back(to_pointer(pointer), JSON::String{value.recompose()});
  });

  // One copy per resource that needs one, however many of its references name
  // the same schema
  std::map<std::pair<Pointer, JSON::String>, JSON::String> placed;
  for (const auto &copy : copies) {
    auto host_container{copy.host};
    host_container.push_back(copy.keyword);
    const std::pair<Pointer, JSON::String> seen{host_container,
                                                copy.identifier};
    const auto hit{placed.find(seen)};
    if (hit == placed.cend()) {
      auto duplicate{
          JSON{get(schema, positional.landings.at(copy.identifier))}};
      // Where it came from said what it was by sitting where it sat, and the
      // place it goes to says something else, so the copy has to say it
      if (copy.carries_its_dialect) {
        mark_dialect_of_every_subschema(duplicate, walker, resolver,
                                        copy.dialect, remaining);
      }

      placed.emplace(seen, embed_schema(schema, host_container, copy.identifier,
                                        std::move(duplicate), callback));
    }

    Pointer target{copy.keyword};
    target.push_back(JSON::String{placed.at(seen)});
    if (copy.tail.has_value()) {
      target.push_back(Pointer{copy.tail.value()});
    }

    rewrites.emplace_back(copy.reference,
                          JSON::String{to_uri(target).recompose()});
  }

  for (const auto &[pointer, value] : rewrites) {
    set(schema, pointer, JSON{value});
  }
}

auto elevate_embedded_resources(
    JSON &remote, JSON &root, const Pointer &container,
    const SchemaBaseDialect remote_dialect, const SchemaWalker &walker,
    const SchemaResolver &resolver, std::string_view default_dialect,
    std::unordered_map<JSON::String, JSON::String> &bundled,
    std::uint64_t &remaining, const SchemaBundleOptions::Callback &callback)
    -> void {
  const auto keyword{definitions_keyword(
      walker,
      vocabularies_with_embedded(remote, resolver, remote_dialect,
                                 declared_dialect(remote, default_dialect)))};
  const JSON::String keyword_string{keyword};
  if (keyword.empty() || !remote.is_object() ||
      !remote.defines(keyword_string) ||
      !remote.at(keyword_string).is_object()) {
    return;
  }

  auto &defs{remote.at(keyword_string)};
  const auto remote_dialect_uri{declared_dialect(remote, default_dialect)};

  // Navigate to the root container once, as it doesn't change per entry
  const JSON *root_container{&root};
  bool container_exists{true};
  for (const auto &token : container) {
    if (!token.is_property() || !root_container->is_object() ||
        !root_container->defines(token.to_property())) {
      container_exists = false;
      break;
    }

    root_container = &root_container->at(token.to_property());
  }

  // A schema finds a meta-schema it pins by looking for it among the locations
  // the specifications reserve, at the top of the document it is reading. A
  // dialect whose own reserved location is neither of those takes its embedded
  // resources somewhere that search does not go, so what is pinned has to stay
  // where whatever pinned it can still find it
  const auto container_is_searched{
      container.size() == 1 && container.back().is_property() &&
      (container.back().to_property() == "$defs" ||
       container.back().to_property() == "definitions")};
  std::unordered_set<JSON::String> pinned;
  if (!container_is_searched) {
    collect_pinned_dialects(remote, pinned);
  }

  std::vector<std::pair<JSON::String, bool>> to_extract;
  std::vector<JSON::String> to_remove;
  for (const auto &entry : defs.as_object()) {
    const auto &key{entry.first};
    const auto &value{entry.second};
    // Only an entry that declares an absolute identifier matching its key can
    // ever be elevated, and framing rejects the fragment-only identifiers that
    // older drafts use for anchors. Rule those out before paying for a frame
    if (!value.is_object()) {
      continue;
    }
    const auto *declared_id{value.try_at("$id")};
    if (declared_id == nullptr) {
      declared_id = value.try_at("id");
    }
    if (declared_id == nullptr || !declared_id->is_string() ||
        declared_id->to_string() != key ||
        !URI{declared_id->to_string()}.is_absolute()) {
      continue;
    }

    // The remote's dialect is what an entry that declares none inherits, so
    // hand it to the frame as the default rather than falling back after
    SchemaFrame entry_frame{SchemaFrame::Mode::Root,
                            value,
                            walker,
                            resolver,
                            remote_dialect_uri,
                            "",
                            SchemaFrame::IdentifierMode::Additional,
                            {EMPTY_WEAK_POINTER},
                            "",
                            remaining};
    charge(remaining, entry_frame);
    const auto &identifier{entry_frame.root()};
    if (identifier.empty() || identifier != key ||
        !URI{identifier}.is_absolute()) {
      continue;
    }

    const JSON::String identifier_string{identifier};
    if (pinned.contains(canonical_uri(identifier))) {
      // Staying put is the only way this one goes on being found, so the
      // document cannot also hold it elsewhere: one identity in two places is
      // not a document anything can read
      if (bundled.contains(identifier_string)) {
        throw SchemaError("A meta-schema that has to stay within the schema "
                          "that pins it cannot be embedded elsewhere too");
      }

      // What stays behind is still in the document under the identity it
      // carries, so a reference to it from anywhere else is answered by what
      // is already here rather than by asking the resolver for another copy
      bundled.emplace(identifier_string, identifier_string);
      continue;
    }

    const auto defines_dialect{value.defines("$schema")};
    if (bundled.contains(identifier_string)) {
      if (container_exists && root_container->is_object()) {
        for (const auto &root_entry : root_container->as_object()) {
          // Same reasoning as above: rule out what cannot match, and what
          // framing would reject, before paying for a frame. What a container
          // calls an entry is no guide to what that entry identifies, since a
          // caller may hold one under a name of its own choosing, so the
          // declared identifier below is what rules an entry in or out
          if (!root_entry.second.is_object()) {
            continue;
          }
          const auto *stored_declared_id{root_entry.second.try_at("$id")};
          if (stored_declared_id == nullptr) {
            stored_declared_id = root_entry.second.try_at("id");
          }
          if (stored_declared_id == nullptr ||
              !stored_declared_id->is_string() ||
              stored_declared_id->to_string() != identifier_string ||
              !URI{stored_declared_id->to_string()}.is_absolute()) {
            continue;
          }

          SchemaFrame stored_frame{SchemaFrame::Mode::Root,
                                   root_entry.second,
                                   walker,
                                   resolver,
                                   remote_dialect_uri,
                                   "",
                                   SchemaFrame::IdentifierMode::Additional,
                                   {EMPTY_WEAK_POINTER},
                                   "",
                                   remaining};
          charge(remaining, stored_frame);
          const auto &stored_id{stored_frame.root()};
          if (stored_id != identifier_string) {
            continue;
          }

          if (defines_dialect) {
            if (root_entry.second != value) {
              throw SchemaError(
                  "Conflicting embedded resources with the same identifier");
            }
          } else {
            // The stored copy of the resource got its dialect stamped on
            // extraction, so compare against a candidate that is stamped in
            // the same way
            auto candidate{value};
            candidate.assign("$schema",
                             JSON{declared_dialect(value, remote_dialect_uri)});
            if (root_entry.second != candidate) {
              throw SchemaError(
                  "Conflicting embedded resources with the same identifier");
            }
          }

          break;
        }
      }

      to_remove.emplace_back(key);
    } else {
      to_extract.emplace_back(key, !defines_dialect);
      bundled.emplace(identifier_string, identifier_string);
    }
  }

  for (const auto &[key, needs_dialect] : to_extract) {
    auto value{std::move(defs.at(key))};
    defs.erase(key);
    // Otherwise the elevated resource would be re-interpreted under the
    // dialect of the schema it gets embedded into, which can differ from
    // the dialect it inherited from the remote it was elevated out of
    if (needs_dialect) {
      value.assign("$schema",
                   JSON{declared_dialect(value, remote_dialect_uri)});
    }

    embed_schema(root, container, key, std::move(value), callback);
  }

  for (const auto &key : to_remove) {
    defs.erase(key);
  }

  if (defs.empty()) {
    remote.erase(JSON::String{keyword});
  }
}

auto embed_references(
    JSON &root, const Pointer &container, JSON &subschema,
    const SchemaWalker &walker, const SchemaResolver &resolver,
    const SchemaBundleOptions::Mode mode, std::string_view default_dialect,
    std::string_view default_id, const SchemaFrame::Paths &paths,
    std::string_view default_base,
    std::unordered_map<JSON::String, JSON::String> &bundled,
    PositionalReferences &positional, std::string_view container_dialect,
    std::uint64_t &remaining, const SchemaBundleOptions::Callback &callback,
    const std::size_t depth = 0) -> void {
  // Create a fresh frame for each schema we analyze to avoid key collisions
  // between different schemas that have references at the same pointer paths
  static const SchemaFrame::Paths NESTED_PATHS{EMPTY_WEAK_POINTER};
  const SchemaFrame frame{
      SchemaFrame::Mode::References, subschema, walker, resolver,
      default_dialect, default_id, SchemaFrame::IdentifierMode::Additional,
      // We only want to frame in "wrapper" mode for the top
      // level object, which is also the only one that the
      // base the caller retrieved it from applies to, as
      // every remote carries the identity it was resolved by
      depth == 0 ? paths : NESTED_PATHS,
      depth == 0 ? default_base : std::string_view{}, remaining};
  charge(remaining, frame);

  // Every schema this reaches, the one the caller handed over as much as every
  // remote gathered into it, is spelled out against the base it was read
  // against, which is what the reference walks below do for a reference and
  // what keeps two identical resources identical
  restate_external_documentation(subschema, frame, walker, resolver);

  // Framing holds the base it resolves against to its canonical spelling, so
  // every reference destination it reports carries that spelling rather than
  // the one the caller wrote. Settling it once here is what lets a reference
  // be compared against it without parsing the base over again for each
  const JSON::String canonical_base{depth == 0 && !default_base.empty()
                                        ? URI::canonicalize(default_base)
                                        : JSON::String{}};

  std::vector<std::tuple<JSON, JSON::String, SchemaBaseDialect>> deferred;
  std::vector<std::pair<Pointer, JSON::String>> ref_rewrites;

  frame.for_each_unresolved_reference([&](const auto &pointer,
                                          const auto &reference) -> void {
    // We don't want to bundle official schemas, as we can expect
    // virtually all implementations to understand them out of the box.
    // Depending on the bundling strategy, we may skip meta-schemas entirely
    if (is_skippable_metaschema_reference(mode, pointer,
                                          reference.destination)) {
      return;
    }

    // If we can't find the destination but there is a base and we can
    // find base, then we are facing an unresolved fragment
    if (!reference.base.empty() && frame.traverse(reference.base).has_value()) {
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    // And so is one whose base is the very document being bundled. A wrapper
    // format puts the schemas it holds below a root that is no schema
    // resource of its own, so nothing framed answers to the document's base
    // and the test above passes it over, even though the fragment is all that
    // leads nowhere
    if (!canonical_base.empty() && reference.base == canonical_base) {
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    if (reference.base.empty()) {
      throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                 "Could not resolve schema reference");
    }

    assert(!reference.base.empty());
    const JSON::String identifier{reference.base};

    if (bundled.contains(identifier)) {
      // A target that went in without a name answers to nothing, so this
      // reference is settled against the finished document rather than here.
      // Spelling it out in full is what lets that pass recognise it, since a
      // relative spelling reads against wherever the schema ends up
      if (positional.unnamed.contains(identifier)) {
        ref_rewrites.emplace_back(to_pointer(pointer),
                                  JSON::String{reference.destination});
        return;
      }

      const auto &mapped_id{bundled.at(identifier)};
      if (mapped_id != identifier) {
        ref_rewrites.emplace_back(
            to_pointer(pointer),
            rebase_reference(mapped_id, reference.fragment));
      } else if (spells_another_document_relatively(reference)) {
        ref_rewrites.emplace_back(to_pointer(pointer),
                                  JSON::String{reference.destination});
      }

      return;
    }

    auto resolved{resolver(identifier)};
    if (!resolved.has_value()) {
      // A base the document itself holds is already answered for above, so
      // one that nobody hands over is a schema from elsewhere that is missing
      throw SchemaResolutionError(
          identifier, "Could not resolve the reference to an external schema");
    }

    // Bundling rewrites the schema before embedding it, so it needs a copy
    // it owns rather than whatever the resolver chose to hand back
    auto remote{std::move(resolved).to_owned()};
    if (!remote.is_object() && !remote.is_boolean()) {
      throw SchemaReferenceError(identifier, to_pointer(pointer),
                                 "The JSON document is not a valid JSON "
                                 "Schema");
    }

    std::optional<SchemaFrame> remote_root_frame;
    try {
      // What a relative identifier resolves against is where the document was
      // retrieved from, which JSON Schema 2020-12 Section 8.2.1 settles: "If
      // this URI is a relative reference, it is resolved against the base URI
      // of the schema resource that contains it", and the resource containing
      // the root of a fetched document is the document itself. Saying nothing
      // here would leave a relative one resolving against whatever document
      // this is bundling into, which names something else
      remote_root_frame.emplace(
          SchemaFrame::Mode::Root, remote, walker, resolver, default_dialect,
          "", SchemaFrame::IdentifierMode::Additional,
          SchemaFrame::Paths{EMPTY_WEAK_POINTER}, identifier, remaining);
      charge(remaining, remote_root_frame.value());
    } catch (const SchemaUnknownBaseDialectError &) {
      throw SchemaReferenceError(identifier, to_pointer(pointer),
                                 "The JSON document is not a valid JSON "
                                 "Schema");
    }

    const auto &remote_root{remote_root_frame->root_location().value().get()};
    // Both of the questions below are of the same vocabularies, and working
    // them out can cost a meta-schema to resolve, so it happens once
    const auto &remote_vocabularies{
        remote_root_frame->vocabularies(remote_root, resolver)};

    // A schema that names a dialect which gives the keyword that named it no
    // meaning cannot be taken at its word, and framing refuses it wherever it
    // meets one. Saying so here is what names the document it came from and
    // the reference that went looking for it, which a refusal from whichever
    // frame happens to reach it first cannot do
    if (remote.is_object() &&
        remote.defines("$schema"sv, JSONSCHEMA_HASH_SCHEMA) &&
        !dialect_defines(walker, remote_vocabularies, "$schema"sv)) {
      throw SchemaReferenceError(identifier, to_pointer(pointer),
                                 "The referenced schema declares a dialect "
                                 "that does not define the keyword that "
                                 "declares it");
    }

    const auto remote_base_dialect{remote_root.base_dialect};
    auto remote_id = remote_root_frame->root();

    // Whether what the reference names within the remote carries a name of its
    // own, which settles how the reference reads without settling where the
    // remote goes
    bool names_a_resource{false};

    // If the reference has a fragment, verify it exists in the remote
    // schema
    if (reference.fragment.has_value()) {
      // A pointer fragment names a place of the document, and the document can
      // answer for that on its own without paying to frame it
      const auto fragment_pointer{
          fragment_to_pointer(URI{reference.destination})};
      const auto *named_value{fragment_pointer.has_value()
                                  ? try_get(remote, fragment_pointer.value())
                                  : nullptr};
      bool exists{named_value != nullptr};

      // An anchor is not a place of the document, and the drafts that spell
      // identifiers as `id` let one look just like a pointer, so a miss above
      // still has to ask the frame. Only the anchors of the remote matter
      // here, rather than every pointer of it
      if (!exists) {
        const SchemaFrame remote_frame{SchemaFrame::Mode::Locations,
                                       remote,
                                       walker,
                                       resolver,
                                       default_dialect,
                                       identifier,
                                       SchemaFrame::IdentifierMode::Additional,
                                       {EMPTY_WEAK_POINTER},
                                       identifier,
                                       remaining};
        charge(remaining, remote_frame);
        exists = remote_frame.traverse(reference.destination).has_value();
      }

      if (!exists) {
        throw SchemaReferenceError(reference.destination, to_pointer(pointer),
                                   "Could not resolve schema reference");
      }

      // Where what it named carries a name of its own, that name is what
      // reaches it from anywhere, and goes on doing so wherever the bundling
      // puts it. Saying where it sits instead would only hold while it sat
      // there, which is not something a reference into a remote can rely on.
      // Nothing that declares no identifier can be answering to one, and
      // ruling that out costs a property lookup where asking the frame costs
      // a walk of the whole remote
      if (named_value != nullptr && named_value->is_object() &&
          (named_value->defines("$id"sv, JSONSCHEMA_HASH_ID) ||
           named_value->defines("id"sv, JSONSCHEMA_HASH_LEGACY_ID))) {
        const SchemaFrame located{SchemaFrame::Mode::Locations,
                                  remote,
                                  walker,
                                  resolver,
                                  default_dialect,
                                  identifier,
                                  SchemaFrame::IdentifierMode::Additional,
                                  {EMPTY_WEAK_POINTER},
                                  "",
                                  remaining};
        charge(remaining, located);
        const auto named{
            located.traverse(to_weak_pointer(fragment_pointer.value()))};
        if (named.has_value() &&
            named.value().get().type == SchemaFrame::LocationType::Resource &&
            to_pointer(named.value().get().pointer) ==
                fragment_pointer.value() &&
            named.value().get().base != identifier) {
          ref_rewrites.emplace_back(to_pointer(pointer),
                                    JSON::String{named.value().get().base});
          names_a_resource = true;
        }
      }
    }

    JSON::String effective_id{remote_id.empty() ? JSON::String{identifier}
                                                : JSON::String{remote_id}};

    // Whether the remote can be made to name itself at all, which decides
    // between giving it a name and pointing at wherever it lands. A boolean
    // has to be restated as the object that says the same thing before it can
    // hold one, which is not something every dialect can say
    const auto remote_can_name_itself{
        dialect_defines_identifier(walker, remote_vocabularies,
                                   remote_base_dialect) &&
        (remote.is_object() ||
         boolean_can_be_restated(walker, remote_vocabularies, remote))};

    const auto remote_dialect_uri{declared_dialect(remote, default_dialect)};

    if (remote_can_name_itself || remote.is_object()) {
      if (remote_can_name_itself) {
        if (remote.is_boolean()) {
          restate_boolean(remote);
        }

        // Otherwise the embedded resource would be re-interpreted under the
        // dialect of the schema it gets embedded into, which can differ from
        // the default dialect that the remote was resolved with
        if (!remote.defines("$schema")) {
          remote.assign("$schema", JSON{remote_dialect_uri});
        }

        schema_reidentify(remote, effective_id, remote_base_dialect);
      } else if (remote_dialect_uri != container_dialect) {
        // Marking is what carries a dialect that no keyword of the schema can
        // declare. Where whatever holds the container already reads under the
        // dialect the remote was read under, inheritance says the same thing
        // for free
        mark_dialect_of_every_subschema(remote, walker, resolver,
                                        remote_dialect_uri, remaining);
      }
    }

    if (names_a_resource) {
      // Settled above, and by a name rather than by a place, so none of what
      // follows has anything to add to it
    } else if (!remote_can_name_itself) {
      // A remote that cannot declare an identifier, whether because its
      // dialect defines no keyword for one or because it says what no object
      // of that dialect can say, is left by the embedding without the name it
      // was resolved by
      assert(effective_id == identifier);
      positional.unnamed.insert(identifier);
      ref_rewrites.emplace_back(to_pointer(pointer),
                                JSON::String{reference.destination});
    } else if (effective_id != identifier) {
      ref_rewrites.emplace_back(
          to_pointer(pointer),
          rebase_reference(effective_id, reference.fragment));
    } else if (spells_another_document_relatively(reference)) {
      ref_rewrites.emplace_back(to_pointer(pointer),
                                JSON::String{reference.destination});
    }

    bundled.emplace(identifier, effective_id);
    bundled.emplace(effective_id, effective_id);
    deferred.emplace_back(std::move(remote), std::move(effective_id),
                          remote_base_dialect);
  });

  // Whatever the walk above reaches is on its way into the document, so its
  // spelling is settled there rather than here. What is left is a reference
  // whose target the document already holds, which still has to name it in a
  // way that does not depend on where the document came from
  frame.for_each_reference(
      [&](const auto, const auto &pointer, const auto &reference) -> void {
        if (!frame.traverse(reference.destination).has_value()) {
          return;
        }

        // A reference that resolved against the identity its own document was
        // read under reads as settled here, and is not. Spelling it out in
        // full is what keeps what it meant, as the embedding takes that
        // identity away and a fragment left on its own would then name a place
        // of whatever document the schema lands in. Where it points is settled
        // once everything has landed
        if (positional.unnamed.contains(JSON::String{reference.base})) {
          ref_rewrites.emplace_back(to_pointer(pointer),
                                    JSON::String{reference.destination});
          return;
        }

        if (spells_another_document_relatively(reference)) {
          ref_rewrites.emplace_back(to_pointer(pointer),
                                    JSON::String{reference.destination});
        }
      });

  for (auto &[rewrite_pointer, rewrite_value] : ref_rewrites) {
    set(subschema, rewrite_pointer, JSON{rewrite_value});
  }

  for (auto &[remote, effective_id, remote_dialect] : deferred) {
    embed_references(root, container, remote, walker, resolver, mode,
                     default_dialect, effective_id, paths, default_base,
                     bundled, positional, container_dialect, remaining,
                     callback, depth + 1);
    elevate_embedded_resources(remote, root, container, remote_dialect, walker,
                               resolver, default_dialect, bundled, remaining,
                               callback);
    const auto key{embed_schema(root, container, effective_id,
                                std::move(remote), callback)};
    if (positional.unnamed.contains(effective_id)) {
      positional.landings.emplace(effective_id, container.concat(key));
    }
  }
}

auto bundle_internal(JSON &schema, const SchemaWalker &walker,
                     const SchemaResolver &resolver,
                     const SchemaBundleOptions::Mode mode,
                     std::string_view default_dialect,
                     std::string_view default_id,
                     const std::optional<Pointer> &default_container,
                     const SchemaFrame::Paths &paths,
                     std::string_view default_base, std::uint64_t &remaining,
                     const SchemaBundleOptions::Callback &callback) -> void {
  // Pre-scan the schema to find any already-embedded schemas and mark them
  // as bundled to avoid re-embedding them. This includes the root schema itself
  // and any schemas already embedded within it
  std::unordered_map<JSON::String, JSON::String> bundled;
  PositionalReferences positional;
  SchemaFrame initial_frame{SchemaFrame::Mode::Locations,
                            schema,
                            walker,
                            resolver,
                            default_dialect,
                            default_id,
                            SchemaFrame::IdentifierMode::Additional,
                            paths,
                            default_base,
                            remaining};
  charge(remaining, initial_frame);
  initial_frame.for_each_resource_uri([&bundled](const auto &uri) -> void {
    bundled.emplace(JSON::String{uri}, JSON::String{uri});
  });
  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    // Whatever bundling embeds has to land somewhere that framing reaches
    // again, or a later pass cannot see it and embeds a second copy. So a
    // container has to be a keyword that the dialect reserves for schema
    // definitions, declared on a schema that the given paths cover. A wrapper
    // format keeps its container outside every schema it frames, where JSON
    // Schema has nothing to say about where things may go
    const auto container{to_weak_pointer(default_container.value())};
    if (std::ranges::any_of(paths, [&container](const auto &path) -> bool {
          return container.starts_with(path);
        })) {
      const auto parent_pointer{default_container.value().initial()};
      const auto parent{
          initial_frame.traverse(to_weak_pointer(parent_pointer))};
      if (!parent.has_value() ||
          (parent.value().get().type != SchemaFrame::LocationType::Resource &&
           parent.value().get().type != SchemaFrame::LocationType::Subschema) ||
          !default_container.value().back().is_property() ||
          walker(default_container.value().back().to_property(),
                 initial_frame.vocabularies(parent.value().get(), resolver))
                  .type != SchemaKeywordType::LocationMembers) {
        throw SchemaContainerError(
            default_container.value(),
            "Could not bundle to a container that the dialect does not "
            "reserve for schema definitions");
      }
    }

    const auto container_parent{initial_frame.traverse(
        to_weak_pointer(default_container.value().initial()))};
    embed_references(
        schema, default_container.value(), schema, walker, resolver, mode,
        default_dialect, default_id, paths, default_base, bundled, positional,
        container_parent.has_value() ? container_parent.value().get().dialect
                                     : default_dialect,
        remaining, callback);
    settle_positional_references(schema, walker, resolver, default_dialect,
                                 default_id, paths, default_base, positional,
                                 remaining, callback);
    return;
  }

  // If the schema identifier is implicit, add it to the top-level of the
  // bundled schema. Otherwise, potential relative references based on this
  // implicit base URI will likely not resolve unless end users happen to
  // know that this implicit base URI is. Note that boolean schemas cannot
  // declare identifiers, so we leave those untouched
  if (!default_id.empty() && schema.is_object()) {
    // Deliberately framed without a default identifier, so that the root
    // comes back empty exactly when the schema declares none of its own
    SchemaFrame declared_frame{SchemaFrame::Mode::Root,
                               schema,
                               walker,
                               resolver,
                               default_dialect,
                               "",
                               SchemaFrame::IdentifierMode::Additional,
                               {EMPTY_WEAK_POINTER},
                               "",
                               remaining};
    charge(remaining, declared_frame);
    // A dialect that reserves no keyword for an identifier cannot be handed
    // one, and the references that the base would have settled are spelled
    // out by the bundling itself rather than left to resolve against it
    const auto &root_location{declared_frame.root_location().value().get()};
    if (declared_frame.root().empty() &&
        dialect_defines_identifier(
            walker, declared_frame.vocabularies(root_location, resolver),
            root_location.base_dialect)) {
      schema_reidentify(schema, default_id, walker, resolver, default_dialect);
    }
  }

  std::optional<SchemaFrame> schema_root_frame;
  try {
    schema_root_frame.emplace(
        SchemaFrame::Mode::Root, schema, walker, resolver, default_dialect,
        default_id, SchemaFrame::IdentifierMode::Additional,
        SchemaFrame::Paths{EMPTY_WEAK_POINTER}, "", remaining);
    charge(remaining, schema_root_frame.value());
  } catch (const SchemaUnknownBaseDialectError &) {
    throw SchemaError("Could not determine how to perform bundling in this "
                      "dialect");
  }

  const auto schema_base_dialect{
      schema_root_frame->root_location().value().get().base_dialect};

  const auto container_keyword{definitions_keyword(
      walker, schema_root_frame->vocabularies(
                  schema_root_frame->root_location().value().get(), resolver))};
  if (container_keyword.empty()) {
    SchemaFrame frame{SchemaFrame::Mode::References,
                      schema,
                      walker,
                      resolver,
                      default_dialect,
                      default_id,
                      SchemaFrame::IdentifierMode::Additional,
                      {EMPTY_WEAK_POINTER},
                      default_base,
                      remaining};
    charge(remaining, frame);
    // This dialect reserves no location for schema definitions, so there is
    // nowhere to put a meta-schema even where the document names one it does
    // not carry. Leaving that as it stands is as far as bundling can get
    // rather than a failure on its part, whatever the mode asked for, whereas
    // a reference to a schema that is merely absent is still something it was
    // asked to resolve and could not
    if (frame.standalone_ignoring_metaschemas()) {
      return;
    }

    throw SchemaError("Could not determine how to perform bundling in this "
                      "dialect");
  }

  if (ref_overrides_adjacent_keywords(schema_base_dialect) &&
      schema.is_object() && schema.defines("$ref")) {
    if (schema.size() == 1) {
      const auto is_draft3{
          schema_base_dialect == SchemaBaseDialect::JSON_SCHEMA_DRAFT_3 ||
          schema_base_dialect == SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER};
      auto branches{JSON::make_array()};
      branches.push_back(schema);
      schema.at("$ref").into(std::move(branches));
      schema.rename("$ref", is_draft3 ? "extends" : "allOf");
    } else {
      throw SchemaError(
          "Cannot bundle a JSON Schema Draft 7 or older with a top-level "
          "`$ref` (which overrides sibling keywords) without introducing "
          "undefined behavior");
    }
  }

  embed_references(schema, {JSON::String{container_keyword}}, schema, walker,
                   resolver, mode, default_dialect, default_id, paths,
                   default_base, bundled, positional,
                   schema_root_frame->root_location().value().get().dialect,
                   remaining, callback);
  settle_positional_references(schema, walker, resolver, default_dialect,
                               default_id, paths, default_base, positional,
                               remaining, callback);
}

} // namespace

auto schema_bundle(JSON &schema, const SchemaWalker &walker,
                   const SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const SchemaBundleOptions &options) -> std::uint64_t {
  auto remaining{options.max_locations};
  try {
    bundle_internal(schema, walker, resolver, options.mode, default_dialect,
                    default_id, options.default_container, options.paths,
                    options.default_base, remaining, options.callback);
  } catch (const SchemaFrameLimitError &) {
    // Every frame spends from what is left rather than from the whole, so the
    // one that ran out reports what it was handed. The caller set the limit
    // for the operation, so that is what the operation reports back
    throw SchemaFrameLimitError{options.max_locations};
  }

  return options.max_locations - remaining;
}

auto schema_bundle(const JSON &schema, const SchemaWalker &walker,
                   const SchemaResolver &resolver,
                   std::string_view default_dialect,
                   std::string_view default_id,
                   const SchemaBundleOptions &options) -> JSON {
  JSON copy = schema;
  [[maybe_unused]] const auto registered{schema_bundle(
      copy, walker, resolver, default_dialect, default_id, options)};
  return copy;
}

} // namespace sourcemeta::core
