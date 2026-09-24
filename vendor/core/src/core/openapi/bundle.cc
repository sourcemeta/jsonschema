#include <sourcemeta/core/openapi.h>

#include "discriminator.h"
#include "document.h"
#include "helpers.h"

#include <cassert>  // assert
#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint64_t
#include <map>      // std::map
#include <optional> // std::optional
#include <set>      // std::set
#include <string>   // std::to_string
#include <utility>  // std::move, std::make_pair, std::pair
#include <vector>   // std::vector

namespace {

// Every walk and every frame that bundling builds spends from the same
// allowance, as how much of it bundling ends up needing is a function of what
// the resolvers hand back rather than of the document the caller passed in. A
// walk reads a document in full before anything charges for it, so what this
// bounds is how many oversized documents are read rather than whether one is
auto charge(std::uint64_t &remaining, const std::size_t locations) -> void {
  assert(locations <= remaining);
  remaining -= locations;
}

// A reference that bundling has to make whole: the member that spells it, what
// it names, and what the position it sits in expects to find there
struct OpenAPIPending {
  sourcemeta::core::Pointer origin;
  sourcemeta::core::JSON::String destination;
  sourcemeta::core::OpenAPIObjectKind expected;
  // Whether a Discriminator Object mapping is what names it. A `$ref` of a
  // Schema Object is one that whatever reads inside a Schema Object follows
  // for itself, and Section 4.8.25 makes a mapping an annotation that nothing
  // there has any account of, so the two part company wherever the shell
  // cannot reach what is named
  bool mapping{false};
  // Whether a Security Requirement Object is what names it. OpenAPI
  // Specification 3.2.1, Section 4.30 lets a name "be the URI of a Security
  // Scheme Object", and that URI is the member the scopes sit under rather than
  // a value of its own, so making it whole renames a member rather than writing
  // to one. Nothing else this brings in is spelled that way
  bool requirement{false};
  // What the reference resolves against, which for everything but a Schema
  // Object is the base of the document that makes it. OpenAPI Specification
  // 3.1.1, Section 4.6 has a relative reference inside a Schema Object use
  // "the nearest parent `$id` as a Base URI" instead
  sourcemeta::core::JSON::String scope;
};

// What the description reaches for and does not hold. A reference that names
// the document being read and lands nowhere is left alone, as OpenAPI
// Specification 3.1.1, Section 4.8.23 holds a `$ref` to the form of a URI and
// says nothing about it having to resolve, so there is nothing to fetch here
// and nothing this specification lets us report
auto pending(const sourcemeta::core::OpenAPIWalk &walk)
    -> std::vector<OpenAPIPending> {
  std::vector<OpenAPIPending> result;
  for (const auto &entry : walk.references) {
    if (walk.locations.contains(entry.second.destination) ||
        sourcemeta::core::openapi_within_document(entry.second.destination,
                                                  walk.base)) {
      continue;
    }

    result.push_back({.origin = entry.second.origin,
                      .destination = entry.second.destination,
                      .expected = entry.second.expected,
                      .scope = walk.base});
  }

  // And so does what a Security Requirement Object names by URI, which OpenAPI
  // Specification 3.2.1, Section 4.30 admits alongside the name of a component:
  // "The name used for each property MUST either correspond to a security
  // scheme declared in the Security Schemes under the Components Object, or be
  // the URI of a Security Scheme Object". The frame keeps these apart from the
  // references above only because a single one of those Objects may name
  // several schemes, which is more than one entry keyed by where it sits
  for (const auto &entry : walk.security_references) {
    if (walk.locations.contains(entry.second.destination) ||
        sourcemeta::core::openapi_within_document(entry.second.destination,
                                                  walk.base)) {
      continue;
    }

    result.push_back({.origin = entry.second.origin,
                      .destination = entry.second.destination,
                      .expected = entry.second.expected,
                      .requirement = true,
                      .scope = walk.base});
  }

  return result;
}

// Where the Object that bundling embeds begins. A target that sits within a
// component of its own document is embedded as that whole component, so that a
// reference to it and a reference deeper into it land on one copy rather than
// on two. OpenAPI Specification 3.1.1, Section 4.8.7 puts every component at
// the same depth, which is what makes the first three tokens the whole test
constexpr std::size_t COMPONENT_DEPTH{3};

// Whether a pointer names a component of the document it belongs to, which is
// what the Components Object holds at a fixed depth of its own
auto is_component(const sourcemeta::core::Pointer &pointer) -> bool {
  return pointer.size() == COMPONENT_DEPTH && pointer.at(0).is_property() &&
         pointer.at(0).to_property() == "components" &&
         pointer.at(1).is_property();
}

auto promote(const sourcemeta::core::Pointer &target)
    -> sourcemeta::core::Pointer {
  if (target.size() < COMPONENT_DEPTH ||
      !target.starts_with(sourcemeta::core::EMPTY_POINTER, "components")) {
    return target;
  }

  return target.slice(0, COMPONENT_DEPTH);
}

// Where the Path Item Object holding an Operation Object sits. OpenAPI
// Specification 3.1.1, Section 4.8.9 puts an Operation Object directly under
// the Path Item Object that holds it, and 3.2.1, Section 4.9 adds
// `additionalOperations`, "A map of additional operations on this path. The
// map key is the HTTP method with the same capitalization that is to be sent
// in the request", which puts a map of its own between the two. So which place
// holds it is what the walk recorded rather than a fixed number of steps up
auto path_item_of(const sourcemeta::core::OpenAPIWalk &remote,
                  const sourcemeta::core::Pointer &operation)
    -> sourcemeta::core::Pointer {
  auto prefix{operation};
  while (!prefix.empty()) {
    prefix = prefix.initial();
    const auto location{remote.locations.find(
        sourcemeta::core::openapi_location_uri(remote.base, prefix))};
    if (location != remote.locations.cend() &&
        location->second.type ==
            sourcemeta::core::OpenAPIObjectKind::PathItem) {
      return prefix;
    }
  }

  return operation.initial();
}

// Which Components Object member an embedded Object goes under. A component
// keeps the member its own document filed it under, which is what that
// document decided the Object is, rather than the member that the kind of the
// reference reaching it would take. The two differ whenever a reference names
// a place within a component rather than the component itself
auto container_of(const sourcemeta::core::Pointer &origin,
                  const sourcemeta::core::OpenAPIObjectKind expected)
    -> sourcemeta::core::JSON::StringView {
  if (is_component(origin)) {
    return origin.at(1).to_property();
  }

  return sourcemeta::core::openapi_component_container(expected);
}

// A reference that leads back to the document being bundled into names a place
// that document already holds, so it is written as a fragment of it rather
// than as the URI that document answers to
auto rebase(const sourcemeta::core::JSON::String &destination,
            const sourcemeta::core::JSON::String &base)
    -> sourcemeta::core::JSON::String {
  if (!sourcemeta::core::openapi_within_document(destination, base)) {
    return destination;
  }

  return destination.substr(base.size());
}

// A Schema Object carries the base of the document it was written in, and
// nothing below the root of a document may declare one of its own. So every
// reference inside a schema that moves is written back as whatever it resolved
// to where it came from, which a schema frame of that document is what settles
auto absolutize_schemas(sourcemeta::core::JSON &value,
                        const sourcemeta::core::JSON &remote_document,
                        const sourcemeta::core::OpenAPIWalk &remote,
                        const sourcemeta::core::Pointer &origin,
                        const sourcemeta::core::Pointer &landing,
                        const sourcemeta::core::JSON::String &dialect,
                        const sourcemeta::core::SchemaWalker &walker,
                        const sourcemeta::core::SchemaResolver &resolver,
                        std::uint64_t &remaining) -> void {
  sourcemeta::core::SchemaFrame::Paths paths;
  for (const auto &entry : remote.locations) {
    if (entry.second.type != sourcemeta::core::OpenAPIObjectKind::Schema ||
        !entry.second.pointer.starts_with(origin)) {
      continue;
    }

    paths.push_back(sourcemeta::core::to_weak_pointer(entry.second.pointer));

    // OpenAPI Specification 3.1.1, Section 4.8.24.5 scopes `jsonSchemaDialect`
    // to "all Schema Objects contained within an OAS document", so a schema
    // that says nothing about the dialect it is written against is read under
    // whichever one the document holding it declares. Moving it to a document
    // that settles on another dialect is what makes it say so itself
    if (remote.dialect == dialect) {
      continue;
    }

    auto &schema{sourcemeta::core::get(
        value, (entry.second.pointer).resolve_from(origin))};
    // Section 4.8.24 lets a Schema Object be a boolean, which declares nothing
    if (schema.is_object() && !schema.defines("$schema")) {
      schema.assign("$schema", sourcemeta::core::JSON{remote.dialect});
    }
  }

  if (paths.empty()) {
    return;
  }

  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      remote_document,
      walker,
      resolver,
      remote.dialect,
      "",
      sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
      paths,
      remote.base,
      remaining};
  charge(remaining, frame.location_count());

  // What a moved schema names by a mapping is resolved where it came from,
  // just as what it names by a reference is
  for (const auto &discriminator : sourcemeta::core::openapi_discriminators(
           remote_document, frame, remote.base, walker, resolver)) {
    if (!discriminator.origin.starts_with(origin)) {
      continue;
    }

    const auto held{(discriminator.origin).resolve_from(origin)};
    const auto *written{sourcemeta::core::try_get(value, held)};
    if (written == nullptr || !written->is_string()) {
      continue;
    }

    // Section 4.3.3 resolves a name against the Components Object of the entry
    // document wherever the Discriminator Object naming it sits, so moving the
    // schema leaves it naming exactly what it named. That holds whether or not
    // the name also happens to lead into what moves, so it is settled before
    // any route is, or a name would be written out as the route it took
    if (sourcemeta::core::openapi_is_component_key(written->to_string())) {
      continue;
    }

    const auto target{frame.traverse(discriminator.destination)};
    if (target.has_value() && target.value().get().pointer.starts_with(
                                  sourcemeta::core::to_weak_pointer(origin))) {
      // What names a place within what moves moves along with it, so long as
      // it names it by something that travels. A name does. RFC 6901 reads a
      // pointer from the root of a document, so one that leads into what moves
      // leads there by a route that moving is exactly what changes, and it is
      // written out as the route to wherever this is headed instead. This is
      // the same distinction a reference of the schema is held to, and a
      // mapping is held to it for the same reason
      //
      // The URI is held rather than made and read in one breath, as what
      // reads its fragment hands back a view into it
      const sourcemeta::core::URI destination{discriminator.destination};
      const auto fragment{destination.fragment()};
      if (!sourcemeta::core::openapi_within_document(discriminator.destination,
                                                     remote.base) ||
          !fragment.has_value() || !fragment.value().starts_with('/')) {
        continue;
      }

      const auto tail{sourcemeta::core::to_pointer(target.value().get().pointer)
                          .resolve_from(origin)};
      sourcemeta::core::set(
          value, held,
          sourcemeta::core::JSON{
              sourcemeta::core::to_uri(landing.concat(tail)).recompose()});
      continue;
    }

    if (written->to_string() != discriminator.destination) {
      sourcemeta::core::set(value, held,
                            sourcemeta::core::JSON{discriminator.destination});
    }
  }

  const auto moved{sourcemeta::core::to_weak_pointer(origin)};

  // OpenAPI Specification 3.1.1, Section 4.6: "Relative references in Schema
  // Objects, including any that appear as `$id` values, use the nearest parent
  // `$id` as a Base URI". So a schema that names itself relatively names
  // something else once it sits in another document, and writing back what it
  // resolved to where it came from is what keeps its identity its own
  frame.for_each_resource([&value, &origin,
                           &moved](const auto &identifier,
                                   const auto &location) -> void {
    if (!location.pointer.starts_with(moved)) {
      return;
    }

    auto &schema{sourcemeta::core::get(
        value,
        (sourcemeta::core::to_pointer(location.pointer)).resolve_from(origin))};
    // Which keyword carries that identity is the dialect's to say rather than
    // whichever one the latest of them happens to use, and a schema that
    // declares none is left without one rather than given one it never had
    if (schema.is_object() &&
        schema.defines(sourcemeta::core::schema_identifier_keyword(
            location.base_dialect))) {
      sourcemeta::core::schema_reidentify(schema, identifier,
                                          location.base_dialect);
    }
  });

  // Section 4.8.24 puts an External Documentation Object on a Schema Object,
  // and Section 4.8.11 makes its `url` "The URI for the target documentation".
  // The frame of the shell never reaches inside a Schema Object, so the one
  // URI a schema carries that is neither a reference nor an identifier is
  // written back here
  frame.for_each_location([&value, &origin, &moved, &frame, &walker,
                           &resolver](const auto, const auto &,
                                      const auto &location) -> void {
    if (!location.pointer.starts_with(moved)) {
      return;
    }

    // Section 4.8.24 lists `externalDocs` among the keywords that the dialect
    // this specification publishes is made of, and Section 4.8.24.5 has a
    // Schema Object read under whichever dialect it declares. So a schema
    // written against one that leaves the keyword out holds no External
    // Documentation Object at all, and what it spells there names nothing that
    // moving could break
    const auto &vocabularies{frame.vocabularies(location, resolver)};
    if (walker("externalDocs", vocabularies).type ==
        sourcemeta::core::SchemaKeywordType::Unknown) {
      return;
    }

    const auto held{
        (sourcemeta::core::to_pointer(location.pointer))
            .resolve_from(origin)
            .concat(sourcemeta::core::Pointer{"externalDocs", "url"})};
    const auto *written{sourcemeta::core::try_get(value, held)};
    if (written == nullptr || !written->is_string()) {
      return;
    }

    const auto absolute{sourcemeta::core::openapi_resolve_uri(
        written->to_string(), sourcemeta::core::JSON::String{location.base})};
    if (absolute.has_value()) {
      sourcemeta::core::set(
          value, held, sourcemeta::core::JSON{absolute.value().recompose()});
    }
  });

  frame.for_each_reference_from(
      moved,
      [&value, &origin, &moved, &landing, &frame, &remote](
          const auto type, const auto &pointer, const auto &reference) -> void {
        // What a dynamic reference names is settled where the schema is
        // evaluated rather than where it sits, so a bare name is left exactly
        // as the description wrote it. Which document that name is looked for
        // in is settled on load though, which JSON Schema Section 8.2.3.2
        // calls out: "Resolved against the current URI base, it produces the
        // URI used as the starting point for runtime resolution. This initial
        // resolution is safe to perform on schema load". So one that names a
        // document of its own is written back like any other reference, and
        // only the name it carries goes on being settled later
        if (type != sourcemeta::core::SchemaReferenceType::Static &&
            reference.original.starts_with('#')) {
          return;
        }

        // A recursive reference names whichever resource it is evaluated
        // within rather than a place, and JSON Schema 2019-09 Section
        // 8.2.4.2.1 leaves it one value to say that with: "The behavior of
        // this keyword is defined only for the value `#`". One of these frames
        // as static wherever no anchor is in scope for it, so writing out what
        // it resolved to is what would make it say something the keyword does
        // not admit, which reading the result back then turns down
        if (!pointer.empty() && pointer.back().is_property() &&
            pointer.back().to_property() == "$recursiveRef") {
          return;
        }

        // Anything already spelled out as what it resolves to is left exactly
        // as the description wrote it
        if (reference.original == reference.destination) {
          return;
        }

        // And so is anything that names a place inside what moves, which is
        // what an anchor that a schema declares and names itself by comes to.
        // Both of them moving together is what keeps the one naming the other,
        // and resolving it against the document it came from is what would
        // break that.
        //
        // A pointer is not like a name that way. RFC 6901 reads one from the
        // root of a document, so one that leads into what moves leads there
        // by a route that moving is exactly what changes. Such a reference is
        // written out as the route to wherever this is headed instead
        const auto target{frame.traverse(reference.destination)};
        if (target.has_value() &&
            target.value().get().pointer.starts_with(moved)) {
          // Which root a pointer counts from is what the reference resolved
          // against rather than how it is spelled. One that resolved against
          // the document is the one moving takes somewhere else. One that
          // resolved against an identifier a schema declares for itself counts
          // from that schema, which moves along whole, and a name is not a
          // pointer at all
          if (!sourcemeta::core::openapi_within_document(reference.destination,
                                                         remote.base) ||
              !reference.fragment.has_value() ||
              !reference.fragment.value().starts_with('/')) {
            return;
          }

          const auto tail{
              sourcemeta::core::to_pointer(target.value().get().pointer)
                  .resolve_from(origin)};
          sourcemeta::core::set(
              value,
              (sourcemeta::core::to_pointer(pointer)).resolve_from(origin),
              sourcemeta::core::JSON{
                  sourcemeta::core::to_uri(landing.concat(tail)).recompose()});
          return;
        }

        sourcemeta::core::set(
            value, (sourcemeta::core::to_pointer(pointer)).resolve_from(origin),
            sourcemeta::core::JSON{reference.destination});
      });
}

// What to call an embedded Object. The name it went by in the document it came
// from is the one a reader would look for, and Section 4.8.7 constrains every
// component key, so a name that does not hold up there is replaced rather than
// carried over. Taking a name the entry document already uses would change
// what an implicit connection resolves to, which is why this only takes a free
// one
// Whatever the Components Object already holds under a member, which is what a
// name has to be free of before anything is written there
auto openapi_component_container_of(
    const sourcemeta::core::JSON &document,
    const sourcemeta::core::JSON::StringView container)
    -> const sourcemeta::core::JSON * {
  const auto *components{document.try_at("components")};
  if (components == nullptr || !components->is_object()) {
    return nullptr;
  }

  const auto *entries{
      components->try_at(sourcemeta::core::JSON::String{container})};
  return entries == nullptr || !entries->is_object() ? nullptr : entries;
}

// Section 4.8.7 admits nothing but letters, digits, dots, hyphens and
// underscores into a component key, so what does not hold up to that is
// dropped rather than carried over, and a name left with nothing is replaced
auto sanitise(const sourcemeta::core::JSON::StringView candidate)
    -> sourcemeta::core::JSON::String {
  sourcemeta::core::JSON::String result;
  for (const auto character : candidate) {
    if (sourcemeta::core::openapi_is_component_key(
            sourcemeta::core::JSON::StringView{&character, 1})) {
      result.push_back(character);
    }
  }

  return result.empty() ? sourcemeta::core::JSON::String{"Bundled"} : result;
}

// Taking a name that the description already gives a meaning to would change
// what an implicit connection resolves to, which is why this only ever takes
// a free one
auto vacant(const sourcemeta::core::JSON &entries,
            sourcemeta::core::JSON::String candidate)
    -> sourcemeta::core::JSON::String {
  if (!entries.defines(candidate)) {
    return candidate;
  }

  candidate.append("_");
  if (!entries.defines(candidate)) {
    return candidate;
  }

  // Going on appending would make the name grow by one for every other name
  // that already took it, which is a description naming one place many times
  // paying for that many times over in the key it ends up with. Counting
  // instead keeps the name to a length the number of them can be written in.
  // Section 4.8.7 admits digits into a component key just as it admits the
  // underscore
  const auto taken{candidate};
  // Trying each number in turn would ask the Components Object about every
  // name that already took this one, and asking it is a walk of everything it
  // holds, so a description naming one place many times over would pay for it
  // many times over again. Closing in on a free number asks it far fewer
  // times. What this settles on is a free number rather than the lowest free
  // one, which a document already holding some of them out of order is what
  // makes the two differ. Either is a name nothing else goes by, which is all
  // a name bundling invents has to be
  std::uint64_t lower{1};
  std::uint64_t upper{2};
  while (entries.defines(taken + std::to_string(upper))) {
    lower = upper;
    upper *= 2;
  }

  // The number above is free and the one below it is taken, and every step
  // keeps both of those true, so the number this ends on is free
  while (upper - lower > 1) {
    const auto middle{lower + ((upper - lower) / 2)};
    if (entries.defines(taken + std::to_string(middle))) {
      lower = middle;
    } else {
      upper = middle;
    }
  }

  candidate = taken + std::to_string(upper);

  return candidate;
}

auto component_name(const sourcemeta::core::JSON &document,
                    const sourcemeta::core::JSON::StringView container,
                    const sourcemeta::core::JSON::StringView source,
                    const sourcemeta::core::Pointer &target,
                    const sourcemeta::core::OpenAPIBundleOptions::Namer &namer)
    -> sourcemeta::core::JSON::String {
  sourcemeta::core::JSON::String candidate{"Bundled"};
  if (namer) {
    candidate = namer(source, container);
  } else if (!target.empty()) {
    // RFC 6901 Section 4 leaves which of an array and an object a
    // reference token addresses to what it is evaluated against, and one of
    // digits reads back as an array index, which spells out to a name the
    // Components Object takes as it stands
    const auto &token{target.back()};
    if (token.is_property()) {
      candidate = token.to_property();
    } else if (token.is_index()) {
      candidate =
          sourcemeta::core::JSON::String{std::to_string(token.to_index())};
    }
  }

  const auto *entries{openapi_component_container_of(document, container)};
  candidate = sanitise(candidate);
  return entries == nullptr ? candidate
                            : vacant(*entries, std::move(candidate));
}

auto embed(sourcemeta::core::JSON &document,
           const sourcemeta::core::JSON::StringView container,
           const sourcemeta::core::JSON::String &name,
           sourcemeta::core::JSON &&value) -> sourcemeta::core::Pointer {
  const sourcemeta::core::JSON::String container_key{container};
  document.assign_if_missing("components",
                             sourcemeta::core::JSON::make_object());
  auto &components{document.at("components")};
  components.assign_if_missing(container_key,
                               sourcemeta::core::JSON::make_object());
  components.at(container_key).assign(name, std::move(value));

  sourcemeta::core::Pointer result;
  result.push_back(sourcemeta::core::JSON::String{"components"});
  result.push_back(container_key);
  result.push_back(name);
  return result;
}

// A Schema Object is addressed by the identifier it declares rather than by
// where it sits, so the name it goes under is a label rather than something a
// reference resolves through, and the last segment of that identifier is the
// part of it a reader looks for
auto schema_name(const sourcemeta::core::JSON &schemas,
                 const sourcemeta::core::JSON::StringView identifier,
                 const sourcemeta::core::OpenAPIBundleOptions::Namer &namer)
    -> sourcemeta::core::JSON::String {
  return vacant(
      schemas,
      sanitise(namer ? sourcemeta::core::JSON::StringView{namer(identifier,
                                                                "schemas")}
                     : identifier.substr(identifier.find_last_of('/') + 1)));
}

// A boolean carries no keyword, so it cannot say who it is, and whatever named
// it by the URI it was found under would go on naming nothing once it sits
// somewhere else. JSON Schema 2020-12 Section 4.3.2 gives each of the two an
// object that says exactly what it says: "true: Always passes validation, as
// if the empty schema `{}`" and "false: Always fails validation, as if the
// schema `{ "not": {} }`". Written that way it can answer to the URI it was
// found under, which leaves every reference that named it naming it still
auto spell_out(sourcemeta::core::JSON &schema) -> void {
  if (!schema.is_boolean()) {
    return;
  }

  const auto permissive{schema.to_boolean()};
  schema = sourcemeta::core::JSON::make_object();
  if (!permissive) {
    schema.assign("not", sourcemeta::core::JSON::make_object());
  }
}

// Bundling what sits inside a Schema Object is JSON Schema's to do, and the
// Components Object holds the one member this specification reserves for
// schemas. Section 4.8.7 constrains the keys of that member, which the
// identifiers that bundling names an embedded schema by do not hold up to, so
// each of them is renamed once it has landed. Nothing resolves through those
// keys, so renaming reaches nothing else
auto bundle_schemas(sourcemeta::core::JSON &document,
                    const sourcemeta::core::OpenAPIWalk &walk,
                    const sourcemeta::core::SchemaWalker &walker,
                    const sourcemeta::core::SchemaResolver &resolver,
                    const sourcemeta::core::JSON::String &base,
                    const std::uint64_t remaining,
                    const sourcemeta::core::OpenAPIBundleOptions &options)
    -> bool {
  const auto &callback{options.callback};
  const auto &namer{options.namer};
  sourcemeta::core::SchemaFrame::Paths paths;
  for (const auto &entry : walk.locations) {
    if (entry.second.type == sourcemeta::core::OpenAPIObjectKind::Schema) {
      paths.push_back(sourcemeta::core::to_weak_pointer(entry.second.pointer));
    }
  }

  if (paths.empty()) {
    return false;
  }

  sourcemeta::core::Pointer container;
  container.push_back(sourcemeta::core::JSON::String{"components"});
  container.push_back(sourcemeta::core::JSON::String{"schemas"});

  // What each schema was resolved by, beside where it landed. Bundling picks a
  // key that is free rather than one that matches, so reading the identifier
  // back off that pointer is only right until two of them collide
  std::vector<
      std::pair<sourcemeta::core::JSON::String, sourcemeta::core::Pointer>>
      landed;
  sourcemeta::core::SchemaBundleOptions schemas_options;
  // Section 4.8.7 holds the `schemas` member of the Components Object to
  // "reusable Schema Objects", and the dialect a schema is written against is
  // not one of those. So what a `$schema` names is left where it is rather
  // than carried into the description, whether or not this specification's own
  // dialect is one that JSON Schema counts as its own
  schemas_options.mode =
      sourcemeta::core::SchemaBundleOptions::Mode::References;
  schemas_options.default_container = container;
  schemas_options.paths = paths;
  schemas_options.default_base = base;
  schemas_options.max_locations = remaining;
  schemas_options.callback =
      [&landed](const std::string_view identifier,
                const sourcemeta::core::WeakPointer &location) -> void {
    landed.emplace_back(sourcemeta::core::JSON::String{identifier},
                        sourcemeta::core::to_pointer(location));
  };

  // Section 4.8.24.5 scopes what the OpenAPI Object sets to the Schema Objects
  // "contained within an OAS document", and says of the rest: "For standalone
  // JSON Schema documents that do not set `$schema` [...] the dialect SHOULD
  // be assumed to be the OAS dialect". A document a resolver hands back is one
  // of those, so it says so itself before anything reads it under the dialect
  // this description happens to have settled on
  const sourcemeta::core::JSON::String standalone{
      sourcemeta::core::openapi_dialect(walk.version)};
  const auto standalone_resolver{
      [&resolver, &standalone, &base](const std::string_view identifier)
          -> sourcemeta::core::SchemaResolverResult {
        auto result{resolver(identifier)};
        // 3.2.1 Section 4.1.2 has every document of a description hold "either
        // an OpenAPI Object or a Schema Object at the root", and what answers
        // here answers as the second. One that is the first instead would be
        // read as a schema, which Appendix G leaves undefined and lets this
        // turn down: "If the same JSON/YAML object is parsed multiple times
        // and the respective contexts require it to be parsed as different
        // Object types, the resulting behavior is implementation defined, and
        // MAY be treated as an error if detected". Turning it down is also
        // what keeps a document of another revision from arriving this way
        if (result.has_value() &&
            sourcemeta::core::openapi_is_document(result.value())) {
          throw sourcemeta::core::OpenAPIReferenceError{
              base, sourcemeta::core::EMPTY_POINTER,
              sourcemeta::core::JSON::String{identifier},
              "This reference must name a schema rather than a document that "
              "holds an OpenAPI Description"};
        }

        if (!result.has_value() || !result.value().is_object() ||
            result.value().defines("$schema")) {
          return result;
        }

        auto owned{std::move(result).to_owned()};
        owned.assign("$schema", sourcemeta::core::JSON{standalone});
        return owned;
      }};

  sourcemeta::core::schema_bundle(document, walker, standalone_resolver,
                                  walk.dialect, "", schemas_options);
  if (landed.empty()) {
    return false;
  }

  auto &schemas{sourcemeta::core::get(document, container)};
  // A resource that travels this way has to say who it is, which is what JSON
  // Schema 2020-12 Section 9.3.1 asks of a Compound Schema Document: "Each
  // embedded JSON Schema Resource MUST identify itself with a URI using the
  // `$id` keyword". A description is not one of those, so nothing binds it
  // here, but a reference that named a schema by URI goes on naming nothing
  // unless the schema carries that URI. Section 4.8.24 lets a Schema Object be
  // a boolean, which carries no keyword at all and so can say nothing, which
  // writing it out as the object saying the same thing settles
  for (const auto &entry : landed) {
    const auto &identifier{entry.first};
    const auto &key{entry.second.back().to_property()};
    auto &schema{schemas.at(key)};
    if (schema.is_boolean()) {
      spell_out(schema);
      schema.assign("$schema", sourcemeta::core::JSON{standalone});
      schema.assign("$id", sourcemeta::core::JSON{identifier});
    }

    const auto name{schema_name(schemas, identifier, namer)};
    schemas.rename(key, sourcemeta::core::JSON::String{name});
    if (callback) {
      callback(identifier, container.concat(name));
    }
  }

  return true;
}

// What the Schema Objects of a description reach for and it does not hold.
// Section 4.3 lists a Schema Object `$ref` among the fields that connect the
// documents of a description, so one of these may name a Schema Object that
// another of those documents declares, which only the shell can reach
auto schema_pending(const sourcemeta::core::JSON &document,
                    const sourcemeta::core::OpenAPIWalk &walk,
                    const sourcemeta::core::SchemaWalker &walker,
                    const sourcemeta::core::SchemaResolver &resolver,
                    std::vector<OpenAPIPending> &result,
                    std::uint64_t &remaining) -> void {
  sourcemeta::core::SchemaFrame::Paths paths;
  for (const auto &entry : walk.locations) {
    if (entry.second.type == sourcemeta::core::OpenAPIObjectKind::Schema) {
      paths.push_back(sourcemeta::core::to_weak_pointer(entry.second.pointer));
    }
  }

  if (paths.empty()) {
    return;
  }

  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      document,
      walker,
      resolver,
      walk.dialect,
      "",
      sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
      paths,
      walk.base,
      remaining};
  charge(remaining, frame.location_count());

  frame.for_each_reference([&frame, &walk,
                            &result](const auto type, const auto &pointer,
                                     const auto &reference) -> void {
    // A dynamic reference names an anchor to be settled where it is
    // evaluated, and a `$schema` names the dialect a schema is written
    // against rather than a part of the description
    if (type != sourcemeta::core::SchemaReferenceType::Static ||
        (!pointer.empty() && pointer.back().is_property() &&
         pointer.back().to_property() == "$schema")) {
      return;
    }

    if (frame.traverse(reference.destination).has_value() ||
        sourcemeta::core::openapi_within_document(reference.destination,
                                                  walk.base)) {
      return;
    }

    // What a reference resolves against is the base of the schema that holds
    // it, which is the nearest identifier an enclosing one declares rather
    // than anything the reference says about itself
    const auto enclosing{frame.traverse(pointer.initial())};
    result.push_back(
        {.origin = sourcemeta::core::to_pointer(pointer),
         .destination = reference.destination,
         .expected = sourcemeta::core::OpenAPIObjectKind::Schema,
         .scope =
             enclosing.has_value()
                 ? sourcemeta::core::JSON::String{enclosing.value().get().base}
                 : walk.base});
  });

  // And so does what a Discriminator Object names by URI, which the frame
  // above does not read because the keyword it sits under belongs to the
  // OpenAPI dialect rather than to JSON Schema
  for (auto &discriminator : sourcemeta::core::openapi_discriminators(
           document, frame, walk.base, walker, resolver)) {
    if (sourcemeta::core::openapi_discriminator_lands(frame, discriminator) ||
        sourcemeta::core::openapi_within_document(discriminator.destination,
                                                  walk.base)) {
      continue;
    }

    result.push_back({.origin = std::move(discriminator.origin),
                      .destination = std::move(discriminator.destination),
                      .expected = sourcemeta::core::OpenAPIObjectKind::Schema,
                      .mapping = true,
                      .scope = std::move(discriminator.scope)});
  }
}

// Whether what a reference names is the kind that the position it sits in
// expects. A Schema Object reference is the one that may name a place inside
// an Object rather than an Object this specification types, as Section 4.6
// reads the fragment as a JSON Pointer and a Schema Object may sit within a
// component of any kind. So what answers for it is the nearest enclosing
// place that framing did record, which has to be a Schema Object itself
auto lands(const sourcemeta::core::OpenAPIWalk &remote,
           const sourcemeta::core::JSON::String &identifier,
           const sourcemeta::core::Pointer &target,
           const sourcemeta::core::OpenAPIObjectKind expected) -> bool {
  auto prefix{target};
  auto exact{true};
  while (true) {
    const auto location{remote.locations.find(
        sourcemeta::core::openapi_location_uri(identifier, prefix))};
    if (location != remote.locations.cend()) {
      if (location->second.type == expected) {
        return true;
      }

      // A Reference Object stands in for whatever the position expects, so it
      // answers for every kind rather than for one of them. It answers for the
      // place it sits at and for nowhere below it though. Section 4.8.23 gives
      // it three fields and every one of them holds a string, and of anything
      // further it says "This object cannot be extended with additional
      // properties, and any properties added SHALL be ignored". So no place
      // named within one is an Object of any kind, and reading the Object it
      // stands in for as the answer would embed something the reference never
      // named
      return exact && location->second.type ==
                          sourcemeta::core::OpenAPIObjectKind::Reference;
    }

    if (expected != sourcemeta::core::OpenAPIObjectKind::Schema ||
        prefix.empty()) {
      return false;
    }

    prefix = prefix.initial();
    exact = false;
  }
}

// The same place, spelled the way the document that holds it spells it. RFC
// 6901 Section 3 makes every reference token a string, and Section 4 leaves
// which of an array and an object it addresses to what it is evaluated
// against, so a
// pointer read out of a URI fragment takes a name made of digits for a place
// in an array. A pointer the walk built knows better, having been there. The
// two then spell one place two ways and compare as two, which every reference
// into a response keyed by a status code would otherwise fall foul of
auto retype(const sourcemeta::core::JSON &document,
            const sourcemeta::core::Pointer &pointer)
    -> sourcemeta::core::Pointer {
  sourcemeta::core::Pointer result;
  const auto *current{&document};
  for (const auto &token : pointer) {
    if (current != nullptr && current->is_array() && token.is_index()) {
      result.push_back(token.to_index());
      current = token.to_index() < current->size()
                    ? &current->at(token.to_index())
                    : nullptr;
      continue;
    }

    auto name{token.is_property() ? token.to_property()
                                  : sourcemeta::core::JSON::String{
                                        std::to_string(token.to_index())}};
    current = current != nullptr && current->is_object() ? current->try_at(name)
                                                         : nullptr;
    result.push_back(std::move(name));
  }

  return result;
}

// Where the Schema Object holding a place named within one sits. A reference
// may name a subschema, which whatever reads JSON Schema reaches through the
// Schema Object holding it rather than a place this specification types. That
// Schema Object carries the base every relative reference under it resolves
// against, so it is what has to travel
auto schema_of(const sourcemeta::core::OpenAPIWalk &remote,
               const sourcemeta::core::Pointer &target)
    -> sourcemeta::core::Pointer {
  auto prefix{target};
  while (true) {
    const auto location{remote.locations.find(
        sourcemeta::core::openapi_location_uri(remote.base, prefix))};
    if (location != remote.locations.cend() &&
        location->second.type == sourcemeta::core::OpenAPIObjectKind::Schema) {
      return prefix;
    }

    if (prefix.empty()) {
      return target;
    }

    prefix = prefix.initial();
  }
}

// What the Schema Objects of another document answer to. OpenAPI Specification
// 3.2.1, Section 4.1.2.1: "Reference targets are defined by fields including
// the OpenAPI Object's `$self` field and the Schema Object's `$id`, `$anchor`,
// and `$dynamicAnchor` keywords". Neither is a document of its
// own, which is why nothing that goes looking for documents finds them, and
// 3.2.1 Section 4.1.2 leaves none of them to be given up on while the document
// that declares one has been read: "Implementations MUST NOT treat a reference
// as unresolvable before completely parsing all documents provided to the
// implementation as possible parts of the OAD"
auto index_schemas(const sourcemeta::core::JSON &remote_document,
                   const sourcemeta::core::OpenAPIWalk &remote,
                   const sourcemeta::core::JSON::String &identifier,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   std::map<sourcemeta::core::JSON::String,
                            std::pair<sourcemeta::core::JSON::String,
                                      sourcemeta::core::Pointer>> &result,
                   std::uint64_t &remaining) -> void {
  sourcemeta::core::SchemaFrame::Paths paths;
  for (const auto &entry : remote.locations) {
    if (entry.second.type == sourcemeta::core::OpenAPIObjectKind::Schema) {
      paths.push_back(sourcemeta::core::to_weak_pointer(entry.second.pointer));
    }
  }

  if (paths.empty()) {
    return;
  }

  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Locations,
      remote_document,
      walker,
      resolver,
      remote.dialect,
      "",
      sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
      paths,
      remote.base,
      remaining};
  charge(remaining, frame.location_count());

  // What a document itself answers to is already how it is reached, and taking
  // that for a place within it would embed a part of it where the whole was
  // named. A document answers to the URI it was retrieved by as well as to the
  // one it names itself with, and 3.2.1 Section 4.1.1 keeps the two apart, so a
  // Schema Object claiming either of them is a Schema Object claiming a whole
  // document
  frame.for_each_resource([&identifier, &remote, &result](
                              const auto &uri, const auto &location) -> void {
    if (uri == remote.base || uri == identifier) {
      return;
    }

    result.emplace(sourcemeta::core::JSON::String{uri},
                   std::make_pair(identifier, sourcemeta::core::to_pointer(
                                                  location.pointer)));
  });

  // A dynamic anchor is settled where a schema is evaluated rather than where
  // it sits, so only the ones that name a place outright are places to reach
  frame.for_each_anchor(
      sourcemeta::core::SchemaReferenceType::Static,
      [&identifier, &result](const auto &uri, const auto &location) -> void {
        result.emplace(sourcemeta::core::JSON::String{uri},
                       std::make_pair(identifier, sourcemeta::core::to_pointer(
                                                      location.pointer)));
      });
}

// Which place of which document a reference naming a Schema Object identifier
// leads to, whether it names the whole of one or a place within one by a
// pointer from it
auto declared_target(
    const std::map<sourcemeta::core::JSON::String,
                   std::pair<sourcemeta::core::JSON::String,
                             sourcemeta::core::Pointer>> &identifiers,
    const sourcemeta::core::JSON::String &destination)
    -> std::optional<
        std::pair<sourcemeta::core::JSON::String, sourcemeta::core::Pointer>> {
  const auto exact{identifiers.find(destination)};
  if (exact != identifiers.cend()) {
    return exact->second;
  }

  const auto resource{
      identifiers.find(sourcemeta::core::openapi_document_uri(destination))};
  if (resource == identifiers.cend()) {
    return std::nullopt;
  }

  // Section 4.6 reads a fragment of this shape as a JSON Pointer, and one that
  // counts from an identifier a schema declares counts from where that schema
  // sits rather than from the root of the document holding it
  const auto tail{sourcemeta::core::fragment_to_pointer(
      sourcemeta::core::URI{destination})};
  if (!tail.has_value()) {
    return std::nullopt;
  }

  return std::make_pair(resource->second.first,
                        resource->second.second.concat(tail.value()));
}

// Which Object of the remote document a reference names, as a pointer. OpenAPI
// Specification 3.1.1, Section 4.6: "If the representation of the referenced
// document is JSON or YAML, then the fragment identifier SHOULD be interpreted
// as a JSON-Pointer as per RFC6901", and a fragment shaped like anything else
// names nothing this can embed
auto target_of(const sourcemeta::core::JSON::String &destination)
    -> std::optional<sourcemeta::core::Pointer> {
  const auto pointer{sourcemeta::core::fragment_to_pointer(
      sourcemeta::core::URI{destination})};
  // An empty fragment names the root of the document, which is the document
  // itself just as naming no fragment at all is
  if (pointer.has_value() && pointer.value().empty()) {
    return std::nullopt;
  }

  return pointer;
}

} // namespace

namespace sourcemeta::core {

namespace {

// Nothing below the root of a document may declare a base of its own, so an
// Object taken out of one goes on reading whatever it carries against
// whichever document it ends up in. Writing back what each of those resolved
// to where it came from is what keeps them naming the same places once that
// Object sits somewhere else
auto absolutize(JSON &value, const OpenAPIWalk &remote, const Pointer &origin,
                const JSON::String &base) -> void {
  for (const auto &entry : remote.references) {
    if (entry.second.origin.starts_with(origin)) {
      set(value, (entry.second.origin).resolve_from(origin),
          JSON{rebase(entry.second.destination, base)});
    }
  }

  // OpenAPI Specification 3.2.1, Section 4.30 has a Security Requirement
  // Object name a Security Scheme Object by the URI of one, which is the one
  // connection of a description spelled as the member that holds the scopes
  // rather than as a value, so this renames rather than writes
  //
  // Every name of one Security Requirement Object is written back at once.
  // Taken one at a time, each rename would read an Object some of whose
  // members had already moved, and making room at a name means giving up
  // whatever sits there, so a member could take the place of one still
  // waiting its turn and carry its scopes off with it
  std::map<JSON::String, Pointer> holders;
  std::map<JSON::String, std::map<JSON::String, JSON::String>> renames;
  for (const auto &entry : remote.security_references) {
    if (!entry.second.origin.starts_with(origin)) {
      continue;
    }

    auto rewritten{rebase(entry.second.destination, base)};
    if (rewritten == entry.second.original) {
      continue;
    }

    const auto held{entry.second.origin.initial()};
    const auto key{openapi_location_uri(remote.base, held)};
    holders.insert_or_assign(key, held);
    renames[key].insert_or_assign(entry.second.original, std::move(rewritten));
  }

  for (const auto &group : renames) {
    const auto &held{holders.at(group.first)};
    auto &requirement{get(value, held.resolve_from(origin))};
    auto rebuilt{JSON::make_object()};
    for (const auto &member : requirement.as_object()) {
      const auto renamed{group.second.find(member.first)};
      const auto &name{renamed == group.second.cend() ? member.first
                                                      : renamed->second};
      // 3.2.1 Section 4.30 says nothing against two names of one Object leading
      // to one scheme, and reading such an Object is no trouble. Writing one
      // back out is what cannot keep both, as the single name they come to is a
      // key that holds one list of scopes rather than two
      const auto *taken{rebuilt.try_at(name)};
      if (taken != nullptr && *taken != member.second) {
        throw OpenAPIError{remote.base, held.concat(JSON::String{member.first}),
                           "A Security Requirement Object that names one "
                           "security scheme twice over cannot keep a list of "
                           "scopes for each of them"};
      }

      rebuilt.assign(name, member.second);
    }

    requirement.into(std::move(rebuilt));
  }

  // And so is every other URI it carries, which the frame does not record as a
  // reference because nothing about the description hangs off where it leads
  for (const auto &entry : remote.locations) {
    if (!entry.second.pointer.starts_with(origin)) {
      continue;
    }

    const auto relative{(entry.second.pointer).resolve_from(origin)};

    // Section 4.8.5 makes a Server Object URL a template rather than a URI
    // reference, and Section 4.8.5 has a relative one name a place "relative to
    // the location where the document containing the Server Object is being
    // served", so it resolves as a template against the document it was
    // written in
    if (entry.second.type == OpenAPIObjectKind::Server) {
      const auto held{relative.concat(JSON::String{"url"})};
      const auto *written{try_get(value, held)};
      if (written != nullptr && written->is_string()) {
        // 3.2.1 Section 4.5.2.1 works this very case through: a document
        // retrieved from one place and naming itself another resolves what it
        // says of the API against where it was found rather than against the
        // name it gave itself
        auto address{openapi_resolve_server_url(
            written->to_string(), remote.retrieval,
            try_get(value, relative.concat(JSON::String{"variables"})))};
        if (!address.has_value()) {
          throw OpenAPIError{
              remote.base, entry.second.pointer.concat(JSON::String{"url"}),
              "A Server Object URL template that leaves what it is "
              "relative to for its variables to decide cannot be read "
              "from another document"};
        }

        set(value, held, JSON{std::move(address.value())});
      }

      continue;
    }

    for (const auto &field : openapi_embedded_uri_fields(entry.second.type)) {
      const auto held{relative.concat(JSON::String{field})};
      const auto *written{try_get(value, held)};
      if (written == nullptr || !written->is_string()) {
        continue;
      }

      const auto absolute{
          openapi_reference_target(written->to_string(), remote)};
      if (absolute.has_value()) {
        set(value, held, JSON{absolute.value().recompose()});
      }
    }
  }
}

// Lift one Object out of the document that declares it and into the entry
// document, writing back everything it carries that would otherwise go on
// resolving against a base that is no longer its own. Where it lands is what
// every reference that reaches it is then written to name
auto adopt(sourcemeta::core::JSON &document,
           const sourcemeta::core::JSON &remote_document,
           const sourcemeta::core::OpenAPIWalk &remote,
           const sourcemeta::core::Pointer &origin,
           const sourcemeta::core::JSON::StringView container,
           const sourcemeta::core::JSON::String &key,
           const sourcemeta::core::JSON::String &base,
           const sourcemeta::core::JSON::String &dialect,
           const sourcemeta::core::SchemaWalker &walker,
           const sourcemeta::core::SchemaResolver &schema_resolver,
           const sourcemeta::core::OpenAPIBundleOptions &options,
           std::uint64_t &remaining) -> sourcemeta::core::Pointer {
  auto value{*try_get(remote_document, origin)};
  absolutize(value, remote, origin, base);

  // Where this is headed is settled before anything it carries is written
  // back, as a reference of its own that names a place within it by a pointer
  // from the root of a document names that pointer rather than the place, and
  // the pointer is what moving changes
  const auto name{
      component_name(document, container, key, origin, options.namer)};
  sourcemeta::core::Pointer landing;
  landing.push_back(sourcemeta::core::JSON::String{"components"});
  landing.push_back(sourcemeta::core::JSON::String{container});
  landing.push_back(name);

  absolutize_schemas(value, remote_document, remote, origin, landing, dialect,
                     walker, schema_resolver, remaining);

  const auto landed{embed(document, container, name, std::move(value))};
  if (options.callback) {
    options.callback(key, landed);
  }

  return landed;
}

// OpenAPI Specification 3.1.1, Section 4.3.3: "It is RECOMMENDED to consider
// all Operation Objects from all parsed documents when resolving any Link
// Object `operationId`. This requires parsing all referenced documents prior
// to determining an `operationId` to be unresolvable". Every document the
// description spans is one bundling has read, so an identifier that named an
// operation of one of them named something before bundling and has to go on
// naming it afterwards. Nothing points at that operation for its own sake, so
// the Path Item holding it is brought along the way one named by an
// `operationRef` is
auto adopt_operations(JSON &document, const OpenAPIWalk &walk,
                      const std::map<JSON::String, JSON> &documents,
                      const std::map<JSON::String, OpenAPIWalk> &walks,
                      std::map<JSON::String, Pointer> &bundled,
                      const JSON::String &base, const SchemaWalker &walker,
                      const SchemaResolver &schema_resolver,
                      const OpenAPIBundleOptions &options,
                      std::uint64_t &remaining) -> bool {
  bool changed{false};
  for (const auto &entry : walk.operation_id_links) {
    if (walk.operation_ids.contains(entry.second)) {
      continue;
    }

    for (const auto &other : walks) {
      const auto named{other.second.operation_ids.find(entry.second)};
      if (named == other.second.operation_ids.cend()) {
        continue;
      }

      const auto operation{other.second.locations.find(named->second)};
      if (operation == other.second.locations.cend()) {
        continue;
      }

      // A Path Item Object is what holds an Operation Object, and the Object
      // the Components Object has a home for
      const auto origin{
          promote(path_item_of(other.second, operation->second.pointer))};
      // Keyed by what the document answers to rather than by where it was
      // found, which is what every other place that fills this map uses.
      // 3.2.1 Section 4.1.1 has a reference name the former, so keying by the
      // latter would embed one Path Item twice over
      const auto key{openapi_location_uri(other.second.base, origin)};
      if (bundled.contains(key)) {
        break;
      }

      bundled.emplace(
          key,
          adopt(document, documents.at(other.first), other.second, origin,
                container_of(origin, OpenAPIObjectKind::PathItem), key, base,
                walk.dialect, walker, schema_resolver, options, remaining));
      changed = true;
      break;
    }
  }

  return changed;
}

// Where a document declares the Tag Object of a given name, which the walk
// records as a place of its own rather than by the name it goes under
auto tag_of(const JSON &document, const OpenAPIWalk &walk,
            const JSON::String &name) -> std::optional<Pointer> {
  for (const auto &entry : walk.locations) {
    if (entry.second.type != OpenAPIObjectKind::Tag) {
      continue;
    }

    const auto *value{try_get(document, entry.second.pointer)};
    if (value == nullptr || !value->is_object()) {
      continue;
    }

    const auto *declared{value->try_at("name")};
    if (declared != nullptr && declared->is_string() &&
        declared->to_string() == name) {
      return entry.second.pointer;
    }
  }

  return std::nullopt;
}

// OpenAPI Specification 3.2.1, Section 4.22, of a Tag Object's `parent`: "The
// `name` of a tag that this tag is nested under. The named tag MUST exist in
// the API description". A description is every document it spans rather than
// the entry one alone, so that tag may be one another document declares, and
// 3.2.1 Section 4.1 only ever puts a Tag Object at the root of a document.
// Bundling moves what the Components Object holds and leaves every root where
// it is, so a name that the description satisfied has to travel along to go on
// being satisfied by what bundling produces. Every document that holds one is
// one bundling has read, just as for a Link Object `operationId`, as a name is
// not something there is anywhere to go and fetch
auto adopt_tags(JSON &document, const OpenAPIWalk &walk,
                const std::map<JSON::String, JSON> &documents,
                const std::map<JSON::String, OpenAPIWalk> &walks,
                const JSON::String &base, const OpenAPIBundleOptions &options)
    -> bool {
  bool changed{false};
  // What this pass has already brought in. The names the walk holds are the
  // ones it read before any of them travelled, and 3.2.1 Section 4.1 has "Each
  // tag name in the list MUST be unique", so two tags nested under one that
  // only another document declares bring it along once between them
  std::set<JSON::String> adopted;
  for (const auto &entry : walk.tag_parents) {
    if (walk.tag_names.contains(entry.second.second) ||
        adopted.contains(entry.second.second)) {
      continue;
    }

    for (const auto &other : walks) {
      const auto &remote_document{documents.at(other.first)};
      const auto declared{
          tag_of(remote_document, other.second, entry.second.second)};
      if (!declared.has_value()) {
        continue;
      }

      auto value{*try_get(remote_document, declared.value())};
      absolutize(value, other.second, declared.value(), base);
      document.assign_if_missing("tags", JSON::make_array());
      auto &tags{document.at("tags")};
      if (options.callback) {
        options.callback(
            openapi_location_uri(other.second.base, declared.value()),
            Pointer{JSON::String{"tags"}, tags.size()});
      }

      tags.push_back(std::move(value));
      adopted.insert(entry.second.second);
      changed = true;
      break;
    }
  }

  return changed;
}

// Section 4.3 counts "the URI form of the Discriminator Object `mapping`
// field" among the fields that identify the referenced elements of a
// description, so one that names a schema of another document names a part of
// that description. The shell reaches such a schema wherever an OpenAPI
// document holds it. One that stands on its own is left here instead, as a
// mapping is an annotation to whatever reads inside a Schema Object and
// nothing there follows it. It goes under the member Section 4.8.7 reserves
// for schemas, keeping the identifier it answers to, so the mapping goes on
// naming what it always named
auto adopt_mappings(
    JSON &document,
    const std::map<JSON::String, std::vector<OpenAPIPending>> &deferred,
    std::map<JSON::String, Pointer> &adopted,
    std::map<JSON::String, Pointer> &identities, const JSON::String &base,
    const SchemaWalker &walker, const SchemaResolver &schema_resolver,
    const JSON::StringView dialect, const OpenAPIBundleOptions &options,
    std::uint64_t &remaining) -> bool {
  bool changed{false};
  // What this pass brought in. Whether a mapping that named it still lands is
  // for the pass after to say, as a mapping may name a place within a schema
  // rather than the whole of it, and such a place is only there to be found
  // once the schema holding it is
  std::set<JSON::String> fresh;
  for (const auto &entry : deferred) {
    const auto identifier{openapi_document_uri(entry.first)};
    // One schema answers for a mapping once, and bringing it in again would
    // not make a mapping that still does not land any likelier to. That it
    // still does not means what the schema says of itself is not what the
    // mapping asked for, which naming where it went is what is left for.
    //
    // Which root that is counted from is what the mapping resolved against,
    // as Section 4.6 has one inside a Schema Object that declares an
    // identifier count from there rather than from the document
    const auto previous{adopted.find(identifier)};
    if (previous != adopted.cend()) {
      if (fresh.contains(identifier)) {
        continue;
      }

      for (const auto &pending : entry.second) {
        JSON named{pending.scope == base
                       ? to_uri(previous->second).recompose()
                       : openapi_location_uri(base, previous->second)};
        const auto *written{try_get(document, pending.origin)};
        if (written != nullptr && *written != named) {
          set(document, pending.origin, std::move(named));
          changed = true;
        }
      }

      continue;
    }

    auto resolved{schema_resolver(identifier)};
    if (!resolved.has_value()) {
      continue;
    }

    // 3.2.1 Section 4.1.2 has a document of a description hold either an
    // OpenAPI Object or a Schema Object at its root, and a mapping names the
    // second of those. Reading the first as one is what Appendix G leaves
    // undefined
    if (openapi_is_document(resolved.value())) {
      throw OpenAPIReferenceError{
          base, entry.second.front().origin, identifier,
          "This mapping must name a schema rather than a document that holds "
          "an OpenAPI Description"};
    }

    auto schema{std::move(resolved).to_owned()};
    // JSON Schema 2020-12 Section 4.3: "A JSON Schema MUST be an object or a
    // boolean", and nothing that reads one can make sense of anything else.
    // What the entry document holds is held to this before it is read, and
    // what a resolver hands back is no different
    if (!schema.is_object() && !schema.is_boolean()) {
      throw OpenAPIReferenceError{
          base, entry.second.front().origin, identifier,
          "A Schema Object must be an object or a boolean"};
    }

    // What a schema answers to is its own to say. JSON Schema 2020-12
    // Section 8.2.1 has an identifier a schema declares be "its canonical
    // [RFC6596] URI", and the base every relative reference
    // under it resolves against, so taking the URI it happened to be fetched
    // by over the one it declares would re-aim every one of those. Only a
    // schema that declares none is given the one it was found under, which is
    // what lets a mapping that named a place within it go on naming that place.
    //
    // What it declares is a URI reference rather than a URI though, and
    // 3.2.1 Section 4.1.2.2 resolves one of those: "The most common base URI
    // source that is used in the event of a missing or relative `$self` (in the
    // OpenAPI Object) and (for Schema Object) `$id` is the retrieval URI". So
    // the URI it was fetched by is what a relative one is read against, which
    // is what makes the identifier that comes back one the description can use
    std::optional<SchemaFrame> root;
    try {
      root.emplace(SchemaFrame::Mode::Root, schema, walker, schema_resolver,
                   JSON::String{dialect}, identifier,
                   SchemaFrame::IdentifierMode::Additional,
                   SchemaFrame::Paths{EMPTY_WEAK_POINTER}, identifier,
                   remaining);
    } catch (const SchemaUnknownBaseDialectError &) {
      // What a schema is written against is what says how to read it, and
      // Section 4.8.24.5 leaves the dialect to whatever the schema names. One
      // that names a dialect nothing here can place is one nothing here can
      // read, which is a different complaint from naming the wrong kind of
      // document altogether
      throw OpenAPIReferenceError{
          base, entry.second.front().origin, identifier,
          "This mapping must name a schema written against a dialect that is "
          "possible to determine"};
    }

    charge(remaining, root.value().location_count());
    const auto &declared{root.value().root()};
    const JSON::String identity{declared.empty() ? identifier
                                                 : JSON::String{declared}};

    // A schema answers to what it declares rather than to the URI it happened
    // to be fetched by, so two mappings reaching one resource by two spellings
    // reach one schema. Landing it a second time would leave the description
    // holding one identifier in two places, which is what whoever frames the
    // result turns down
    // What a schema declares is its own, and what it was fetched by is the
    // description's. Reading one against the other would take a mapping whose
    // URI happens to spell what another schema calls itself for a second
    // mention of that schema, so the two are kept apart
    const auto same{identities.find(identity)};
    if (same != identities.cend()) {
      adopted.emplace(identifier, same->second);
      continue;
    }
    // Section 4.8.24.5: "For standalone JSON Schema documents that do not set
    // `$schema` [...] the dialect SHOULD be assumed to be the OAS dialect"
    spell_out(schema);
    if (!schema.defines("$schema")) {
      schema.assign("$schema", JSON{dialect});
    }

    schema_reidentify(schema, identity,
                      root.value().root_location().value().get().base_dialect);

    const auto *schemas{openapi_component_container_of(document, "schemas"sv)};
    const auto name{
        schema_name(schemas == nullptr ? JSON::make_object() : *schemas,
                    identifier, options.namer)};
    const auto landed{embed(document, "schemas"sv, name, std::move(schema))};
    adopted.emplace(identifier, landed);
    identities.emplace(identity, landed);
    fresh.insert(identifier);
    if (options.callback) {
      options.callback(identifier, landed);
    }

    changed = true;
  }

  return changed;
}

auto bundle_internal(JSON &document, const SchemaWalker &walker,
                     const SchemaResolver &schema_resolver,
                     const OpenAPIResolver &resolver,
                     const OpenAPIBundleOptions &options,
                     std::uint64_t &remaining) -> void {
  // Where the document was retrieved from, which is only what it answers to
  // until it says otherwise. OpenAPI Specification 3.2.1, Section 4.1 lets one
  // name itself with `$self`, "which also serves as its base URI", so what
  // every place of this document is named by is what its own analysis settled
  // on rather than what the caller handed over
  const auto retrieval{openapi_canonical_base(options.default_base)};
  // Where each component that bundling embedded ended up, keyed by the place
  // it came from. A description that reaches for one place twice embeds it
  // once, which is also what keeps a cycle of documents from going round
  std::map<JSON::String, Pointer> bundled;
  // A document that more than one reference reaches is read once. What it
  // holds cannot change between one reference and the next, and reading it
  // again would charge the allowance twice for the same thing
  std::map<JSON::String, JSON> documents;
  std::map<JSON::String, OpenAPIWalk> walks;
  // The documents that the shell cannot reach, which only a Schema Object may
  // name and which it is left free to go on naming
  std::set<JSON::String> unavailable;
  // What every Schema Object of a document already read answers to, beside the
  // document holding it and where in it it sits. A reference naming one of
  // these names a place rather than a document, so nothing that goes looking
  // for documents would ever find it
  std::map<JSON::String, std::pair<JSON::String, Pointer>> identifiers;
  // And of those, the schemas that a Discriminator Object mapping is what
  // names, which nothing but this brings in, along with the ones it already did
  std::map<JSON::String, std::vector<OpenAPIPending>> deferred;
  std::map<JSON::String, Pointer> adopted;
  // And where each of them ended up, against the identifier it declares for
  // itself rather than the URI it was reached by. JSON Schema 2020-12 Section
  // 8.2.1 has the first be "its canonical [RFC6596] URI" and says nothing of
  // what is served at the second, so one schema reached by two URIs is told
  // from two schemas that happen to spell each other's names
  std::map<JSON::String, Pointer> identities;
  // The names the description gave before bundling moved anything. Section
  // 4.1.2.3 resolves the names a referenced document uses from the entry
  // document, and bundling embedding a Security Scheme Object puts a name
  // there that the description never had. Letting that answer for a document
  // read afterwards would decide, by nothing but how many references away it
  // sits, that an operation requires a credential its own document never named
  OpenAPIWalk names;
  bool named{false};

  while (true) {
    const auto walk{openapi_analyse(document, retrieval, remaining)};
    const auto &base{walk.base};
    if (!named) {
      names.security_schemes = walk.security_schemes;
      names.tags = walk.tags;
      names.tag_names = walk.tag_names;
      named = true;
    }

    charge(remaining, walk.locations.size());
    auto unresolved{pending(walk)};
    // A Schema Object may name one that another document of the description
    // declares, which the shell is what reaches rather than anything that
    // reads inside a Schema Object
    schema_pending(document, walk, walker, schema_resolver, unresolved,
                   remaining);

    bool changed{false};
    for (const auto &reference : unresolved) {
      // Only the shell knows which documents an OpenAPI Description spans. A
      // Schema Object naming something this does not reach is left exactly as
      // it was written, for whatever reads inside one to resolve
      const auto names_a_schema{reference.expected ==
                                OpenAPIObjectKind::Schema};

      // Where a reference leads before the document holding it has been read,
      // which is all a fragment shaped like a pointer ever needs
      const auto pointed{target_of(reference.destination)};
      // Which document it leads to, if it names one at all
      const auto holder{openapi_document_uri(reference.destination)};

      // What another document's Schema Object declares for itself is a name
      // for a place of that document, so a reference naming one is a reference
      // into it rather than one to go looking for a document of that name.
      //
      // Whatever the description can reach as a document answers ahead of
      // that, as 3.2.1 Section 4.1.1 has a reference name a document by the URI
      // that document answers to, and a Schema Object is free to declare an
      // identifier that collides with one. So this is asked where a fragment
      // names no place to begin with, and otherwise only once the document has
      // turned out not to be one
      auto declared{names_a_schema && (!pointed.has_value() ||
                                       unavailable.contains(holder))
                        ? declared_target(identifiers, reference.destination)
                        : std::nullopt};

      const auto initial{declared.has_value()
                             ? std::optional<Pointer>{declared.value().second}
                             : pointed};
      // A fragment shaped like anything else names an identifier or an anchor
      // rather than a place, and which document declares one is only settled
      // by framing the documents the description spans. 3.2.1 Section 4.1.2
      // leaves none of those to be given up on before that has happened, so a
      // reference of this shape goes on to have its document read rather than
      // being left here
      const URI destination{reference.destination};
      const auto names_an_anchor{names_a_schema && !initial.has_value() &&
                                 destination.fragment().has_value() &&
                                 !destination.fragment().value().empty()};

      if (!initial.has_value() && !names_an_anchor) {
        if (names_a_schema) {
          if (reference.mapping) {
            deferred[reference.destination].push_back(reference);
          }

          continue;
        }

        throw OpenAPIReferenceError{base, reference.origin,
                                    reference.destination,
                                    "This reference must name a place within "
                                    "the document it points at"};
      }

      const auto identifier{declared.has_value() ? declared.value().first
                                                 : holder};

      if (names_a_schema && !declared.has_value() &&
          unavailable.contains(identifier)) {
        continue;
      }

      if (!documents.contains(identifier)) {
        auto resolved{resolver(identifier)};
        if (!resolved.has_value()) {
          if (names_a_schema) {
            unavailable.insert(identifier);
            if (reference.mapping) {
              deferred[reference.destination].push_back(reference);
            }

            continue;
          }

          throw OpenAPIResolutionError{
              base, reference.origin, identifier,
              "Could not resolve the reference to an external document"};
        }

        auto candidate{std::move(resolved).to_owned()};
        // OpenAPI Specification 3.2.1, Section 4.1.2: "all documents in an
        // OAD MUST have either an OpenAPI Object or a Schema Object at the
        // root, and MUST be parsed as complete documents". A Schema Object
        // at the root is what a Schema Object reference may name, and that
        // document is one a JSON Schema implementation reads rather than
        // this. No revision of 3.1 carries that sentence, and what it carries
        // instead is the choice Section 4.3.1 leaves open, so declining to
        // read a document of some other shape is one rule for both revisions
        if (!openapi_is_document(candidate)) {
          if (names_a_schema) {
            unavailable.insert(identifier);
            if (reference.mapping) {
              deferred[reference.destination].push_back(reference);
            }

            continue;
          }

          throw OpenAPIReferenceError{base, reference.origin, identifier,
                                      "This reference must name a document "
                                      "that holds an OpenAPI Description"};
        }

        // Nothing in the specification asks for this. It says what a
        // description may span and says nothing of the revisions the documents
        // it spans declare, so turning one of these down is a choice this
        // makes rather than a rule it follows.
        //
        // What makes the choice is where bundling ends. Section 4.1 has one
        // document declare one revision, so everything the result holds has to
        // be what that one revision can express, and there is no revision to
        // pick that can express both. A 3.2 document holds fields that 3.1 has
        // no way of spelling, and 3.2.1 Section 2.1 leaves no room to assume
        // the other direction is safe either: "Occasionally, non-backwards
        // compatible changes may be made in `minor` versions of the OAS where
        // impact is believed to be low relative to the benefit provided". So
        // the choice is between turning such a description down and handing
        // back a document that says it is one revision while holding what
        // another one means.
        //
        // What the patch component says is no part of this, as 3.2.1
        // Section 2.1 makes a revision the `major`.`minor` pair alone,
        // which 3.1.1 says the same of under Section 4.1
        const auto revision{openapi_version(candidate)};
        if (revision.has_value() && revision.value() != walk.version) {
          throw OpenAPIReferenceError{
              base, reference.origin, identifier,
              "This reference must name a document of the same OpenAPI "
              "Specification revision"};
        }

        // The walk keeps a view into the document it read, so the document
        // takes its place before anything walks it
        const auto &held{
            documents.emplace(identifier, std::move(candidate)).first->second};
        auto analysis{openapi_analyse(held, identifier, remaining, &names)};
        charge(remaining, analysis.locations.size());
        const auto &recorded{
            walks.emplace(identifier, std::move(analysis)).first->second};
        index_schemas(held, recorded, identifier, walker, schema_resolver,
                      identifiers, remaining);
      }

      // A fragment shaped like anything but a pointer names an identifier or
      // an anchor rather than a place, which only framing the document that
      // declares it settles. 3.2.1 Section 4.1.2 leaves none of those to be
      // given up on before that document has been read, so this is asked again
      // now that it has been. A fragment that is a pointer already named a
      // place of the document just read, and nothing another document declares
      // for itself is to take that place from it
      if (names_a_schema && !declared.has_value() && !pointed.has_value()) {
        declared = declared_target(identifiers, reference.destination);
      }

      // A document that holds an OpenAPI Description is never what a reference
      // from the shell expects to find, so one naming a whole document has
      // landed on the wrong kind of thing
      const auto target{declared.has_value()
                            ? std::optional<Pointer>{declared.value().second}
                            : target_of(reference.destination)};
      if (!target.has_value()) {
        if (names_a_schema) {
          if (reference.mapping) {
            deferred[reference.destination].push_back(reference);
          }

          continue;
        }

        throw OpenAPIReferenceError{base, reference.origin,
                                    reference.destination,
                                    "This reference must name a place within "
                                    "the document it points at"};
      }

      const auto &remote_document{documents.at(identifier)};
      const auto &remote{walks.at(identifier)};
      // Spelled as the document that holds it spells it, before anything
      // compares it against what the walk of that document recorded
      const auto spelled{retype(remote_document, target.value())};
      // What a reference names is held to the kind the position it sits in
      // expects, however many references reach the Object that holds it. A
      // second one that named nothing would otherwise be written out as a
      // place the result does not hold
      if (!lands(remote, remote.base, spelled, reference.expected)) {
        throw OpenAPIReferenceError{
            base, reference.origin, reference.destination,
            "This reference must name an Object of the kind that the "
            "position it sits in expects"};
      }

      // An Operation Object is the one kind the Components Object has no home
      // for, so what gets embedded is the Path Item Object holding it and the
      // reference goes on reaching its operation through that
      const auto names_an_operation{reference.expected ==
                                    OpenAPIObjectKind::Operation};
      const auto origin{promote(names_an_operation
                                    ? path_item_of(remote, spelled)
                                : names_a_schema ? schema_of(remote, spelled)
                                                 : spelled)};
      const auto container{
          container_of(origin, names_an_operation ? OpenAPIObjectKind::PathItem
                                                  : reference.expected)};
      // Every kind that a reference position expects has a home of its own
      // once an Operation Object is reached through the Path Item holding it
      assert(!container.empty());

      // Where a document was retrieved from is how a resolver is asked for it,
      // and what it answers to is its own to say. OpenAPI Specification 3.2.1,
      // Section 4.1.1 lets a document declare the latter and requires the two
      // to be told apart: "references MUST use the target document's `$self`
      // URI if the `$self` field is present". So every place of it is named by
      // the base its own analysis settled on rather than by where it was found
      const auto key{openapi_location_uri(remote.base, origin)};

      if (!bundled.contains(key)) {
        bundled.emplace(key, adopt(document, remote_document, remote, origin,
                                   container, key, base, walk.dialect, walker,
                                   schema_resolver, options, remaining));
      }

      // The reference is rewritten as a fragment of the document it now sits
      // in, rather than as the URI that document answers to, so that bundling
      // leaves behind an output that keeps working wherever it is moved to
      const auto landed{spelled.rebase(origin, bundled.at(key))};

      // What a Security Requirement Object names is the member the scopes sit
      // under, so what makes it whole is renaming that member to whatever the
      // scheme is called once it sits here, which carries the scopes across
      // untouched. 3.2.1 Section 4.30 then reads the result as a component name
      // rather than as a URI: "Property names that are identical to a
      // component name under the Components Object MUST be treated as a
      // component name", and the name was taken free of that Object for it
      if (reference.requirement) {
        auto &requirement{get(document, reference.origin.initial())};
        const auto &member{reference.origin.back().to_property()};
        JSON::String name{landed.back().to_property()};
        const auto *taken{requirement.try_at(name)};
        if (taken != nullptr && *taken != requirement.at(member)) {
          throw OpenAPIError{base, reference.origin,
                             "A Security Requirement Object that names one "
                             "security scheme twice over cannot keep a list of "
                             "scopes for each of them"};
        }

        requirement.rename(member, std::move(name));
        changed = true;
        continue;
      }

      // A reference that resolves against the document it sits in is written
      // as a fragment of it, which is what keeps what bundling produces
      // working wherever it is moved to. One that resolves against something
      // else, which is a Schema Object that declares an identifier of its own,
      // would name a place within that identifier instead, so it cannot be
      // written that way
      if (reference.scope == base) {
        set(document, reference.origin, JSON{to_uri(landed).recompose()});
        changed = true;
        continue;
      }

      // What such a reference names instead is what the schema it leads to
      // says of itself, wherever that schema says anything. JSON Schema
      // Section 9.3.1 has a reference to an embedded resource "resolve to a
      // schema using the `$id` of an embedded Schema Resource", and an
      // identifier holds wherever the description ends up while a place in a
      // document does not. Only a schema that declares none leaves the
      // document itself as the sole way to name where it leads
      const auto *reached{try_get(document, landed)};
      const auto *identity{reached == nullptr || !reached->is_object()
                               ? nullptr
                               : reached->try_at("$id")};
      set(document, reference.origin,
          identity != nullptr && identity->is_string()
              ? JSON{identity->to_string()}
              : JSON{openapi_location_uri(base, landed)});
      changed = true;
    }

    // A pass that leaves the document as it found it is one that has nothing
    // left to bring in, which counts a reference left for a JSON Schema
    // implementation as nothing
    if (!changed) {
      // What the shell reaches is settled before this, as an operation is
      // brought in for the sake of a name rather than of a reference and only
      // the documents a reference reached are ones to look through
      if (adopt_operations(document, walk, documents, walks, bundled, base,
                           walker, schema_resolver, options, remaining)) {
        continue;
      }

      // And so is the tag a Tag Object is nested under, which is a name the
      // description settles rather than a reference to go and follow
      if (adopt_tags(document, walk, documents, walks, base, options)) {
        continue;
      }

      // And so is what a mapping names, which is settled after the references
      // are, as bringing one schema in may be what lets the next be read
      if (adopt_mappings(document, deferred, adopted, identities, base, walker,
                         schema_resolver, openapi_dialect(walk.version),
                         options, remaining)) {
        deferred.clear();
        continue;
      }

      // What is left is one document that holds every OpenAPI document the
      // description spanned, as the only ones bundling leaves out are the ones
      // a Schema Object may name, which hold no Tag Object and no Operation
      // Object to name. So the names that Section 4.3.3 has resolve across the
      // whole of a description are ones there is now an answer for, and
      // leaving a description that has none to be turned down by whoever
      // frames it next would be to hand back a bundle that does not describe
      // anything
      openapi_check_operation_id_links(walk, walk.locations);
      openapi_check_tag_parents(walk, walk.locations, true);
      // And so are the path parameters a templated path corresponds to, which
      // the projection is what settles. What it works out is of no use here,
      // as bundling moves what a description holds rather than reporting what
      // it exposes, but a description that cannot be projected is one this
      // would otherwise hand back for the next reader to turn down
      [[maybe_unused]] const auto operations{openapi_project(walk)};

      // What sits inside a Schema Object is JSON Schema's to bring in, and a
      // schema that lands may hold a Discriminator Object naming another,
      // which nothing has read yet. So this goes round once more rather than
      // being the last thing that happens.
      //
      // Going round again is safe as well as needed. Every schema that lands
      // says who it is, and the one kind that cannot, a boolean, has whatever
      // named it written out to name where it went, so a second pass finds
      // nothing left outside to ask for rather than landing a second copy
      if (bundle_schemas(document, walk, walker, schema_resolver, base,
                         remaining, options)) {
        continue;
      }

      return;
    }
  }
}

} // namespace

auto openapi_bundle(JSON &document, const SchemaWalker &walker,
                    const SchemaResolver &schema_resolver,
                    const OpenAPIResolver &resolver,
                    const OpenAPIBundleOptions &options) -> void {
  auto remaining{options.max_locations};
  try {
    bundle_internal(document, walker, schema_resolver, resolver, options,
                    remaining);
  } catch (const OpenAPIFrameLimitError &) {
    throw OpenAPIBundleLimitError{options.max_locations};
  } catch (const SchemaFrameLimitError &) {
    // Every frame spends from what is left rather than from the whole, so the
    // one that ran out reports what it was handed. The caller set the
    // allowance for the operation, so that is what the operation reports back
    throw OpenAPIBundleLimitError{options.max_locations};
  }
}

auto openapi_bundle(const JSON &document, const SchemaWalker &walker,
                    const SchemaResolver &schema_resolver,
                    const OpenAPIResolver &resolver,
                    const OpenAPIBundleOptions &options) -> JSON {
  JSON copy{document};
  openapi_bundle(copy, walker, schema_resolver, resolver, options);
  return copy;
}

} // namespace sourcemeta::core
