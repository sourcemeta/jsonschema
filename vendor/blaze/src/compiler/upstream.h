#ifndef SOURCEMETA_BLAZE_COMPILER_UPSTREAM_H_
#define SOURCEMETA_BLAZE_COMPILER_UPSTREAM_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::ranges::all_of, std::max
#include <cassert>       // assert
#include <concepts>      // std::predicate
#include <cstdint>       // std::uint64_t
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::to_underlying
#include <variant>       // std::get, std::get_if
#include <vector>        // std::vector

// Everything here is staged for Core, which is why it is declared in Core's
// namespace. Nothing in this file may know anything about Blaze, and each entry
// names the Core home it is waiting on, so that landing it upstream is a
// deletion of the entry rather than a rewrite of its call sites. A Core upgrade
// that lands an entry under this same name and signature is a redefinition
// error, which is the outcome we want. One that lands it under another name, or
// another signature, leaves a silent duplicate behind, so read this file
// against the Core changelog on every upgrade
//
// An entry whose home is a member of a Core class is the exception, as a header
// that may not touch Core cannot stage a member. Each of those names the member
// it is waiting to become, and landing it does rewrite its call sites

namespace sourcemeta::core {

// Waiting on: the `jsonschema` module, beside the walker's instance type sets

// The instance types that a `type` name stands for. A name that JSON Schema
// does not define stands for no type at all, so it widens nothing
inline auto schema_type_set(const std::string_view type) -> JSON::TypeSet {
  JSON::TypeSet result;
  if (type == "null") {
    result.set(std::to_underlying(JSON::Type::Null));
  } else if (type == "boolean") {
    result.set(std::to_underlying(JSON::Type::Boolean));
  } else if (type == "object") {
    result.set(std::to_underlying(JSON::Type::Object));
  } else if (type == "array") {
    result.set(std::to_underlying(JSON::Type::Array));
  } else if (type == "number") {
    result.set(std::to_underlying(JSON::Type::Integer));
    result.set(std::to_underlying(JSON::Type::Real));
    result.set(std::to_underlying(JSON::Type::Decimal));
  } else if (type == "integer") {
    result.set(std::to_underlying(JSON::Type::Integer));
  } else if (type == "string") {
    result.set(std::to_underlying(JSON::Type::String));
  }

  return result;
}

// The instance types that a `type` keyword value permits, where an array of
// names permits the union of what each one stands for
inline auto schema_type_set(const JSON &type) -> JSON::TypeSet {
  if (type.is_string()) {
    return schema_type_set(type.to_string());
  }

  JSON::TypeSet result;
  if (type.is_array()) {
    for (const auto &item : type.as_array()) {
      if (item.is_string()) {
        result |= schema_type_set(item.to_string());
      }
    }
  }

  return result;
}

// Waiting on: the `jsonschema` module

// Draft 6 introduced boolean schemas. Draft 4 and earlier have none, and the
// OpenAPI 3.0 Schema Object has none either
inline auto schema_is_schema(const JSON &value,
                             const SchemaVocabularies &vocabularies) -> bool {
  using Known = SchemaVocabularies::Known;
  return value.is_object() ||
         (value.is_boolean() &&
          !vocabularies.contains_any(
              {Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
               Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
               Known::OPENAPI_3_0_BASE}));
}

// Draft 3 and Draft 4 read "integer" as a number written without a fractional
// part, so a bound like `2.0` is not one there. Draft 6 onwards widened it to
// any number whose fractional part is zero. The OpenAPI 3.0 Schema Object
// answers with Draft 4, as its meta-schema is a Draft 4 document.
//
// Note that OpenAPI 3.0.4 says the opposite in prose, that "both `1` and
// `1.0` are equivalent, and are both considered to be integers", reversing
// what 3.0.3 said. We keep the Draft 4 reading for every dated OpenAPI 3.0
// dialect regardless, because the Schema Object extends the Draft 4 base
// dialect and so is the base dialect's question to answer, not a patch
// release's to redefine
inline auto
schema_integral_reals_are_integers(const SchemaVocabularies &vocabularies)
    -> bool {
  using Known = SchemaVocabularies::Known;
  return !vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_3, Known::JSON_SCHEMA_DRAFT_3_HYPER,
       Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
       Known::OPENAPI_3_0_BASE});
}

// Waiting on: the `jsonschema` module, where it lived until Core #980, and
// better still a field of `SchemaWalkerResult`, as the answer is fixed per
// keyword and vocabulary set while this recomputes it on every call

// A keyword evaluates after everything it reads, so its priority is one above
// the highest of what it depends on
inline auto schema_keyword_priority(const std::string_view keyword,
                                    const SchemaVocabularies &vocabularies,
                                    const SchemaWalker &walker)
    -> std::uint64_t {
  const auto &metadata{walker(keyword, vocabularies)};
  std::uint64_t priority{0};
  for (const auto &dependency : metadata.dependencies) {
    priority =
        std::max(priority,
                 schema_keyword_priority(dependency, vocabularies, walker) + 1);
  }

  for (const auto &dependency : metadata.order_dependencies) {
    priority =
        std::max(priority,
                 schema_keyword_priority(dependency, vocabularies, walker) + 1);
  }

  return priority;
}

// Waiting on: `SchemaVocabularies`, which declares both of these types and
// nothing that crosses between them, as a static member of it rather than as a
// free function, which would sit beside the `vocabulary_uri` that Core already
// has for converting the other way

// A walker only views the custom vocabulary URIs it reports, as its table
// points at static storage, so keeping one past the walker means taking a copy
inline auto schema_vocabulary_uri(const SchemaVocabularies::URIView &vocabulary)
    -> SchemaVocabularies::URI {
  const auto *known{std::get_if<SchemaVocabularies::Known>(&vocabulary)};
  if (known != nullptr) {
    return *known;
  }

  return JSON::String{std::get<std::string_view>(vocabulary)};
}

// Waiting on: `SchemaFrame`, as a field of `SchemaFrame::Reference` set while
// framing, which is where the answer is known without deriving it from a
// pointer. Core repeats this very test when it decides whether a schema stands
// alone

// A `$schema` reference points at a meta-schema rather than into the schema
// itself, so it describes the document rather than what the document says
inline auto schema_is_metaschema_reference(const WeakPointer &origin) -> bool {
  return !origin.empty() && origin.back().is_property() &&
         origin.back().to_property() == "$schema";
}

// Waiting on: `SchemaFrame`, as `SchemaFrame::TargetType` and a member that
// returns them, derived into the frame's cache the way its vocabularies are. A
// field of `SchemaFrame::Location` would read better where this is asked, as a
// caller holds one already, but the answer only exists once every reference is
// known, so it would stand for nothing in the modes that record none

// What a reference target gets evaluated against. Both hold of a target that
// references of both kinds arrive at
struct SchemaFrameTargetType {
  bool property_name{false};
  bool instance{false};
};

// One table over the whole frame, computed once, so that asking what a given
// target is evaluated against costs a lookup rather than a walk of everything
// that reaches it. The frame already answers the lexical half of this on its
// locations, as a subschema written inside `propertyNames` carries it, and what
// is left is what arrives through a reference: a target evaluated against
// property names passes that on to everything it references in turn, so the
// answer is a closure rather than a tally of the references that arrive
// directly
inline auto schema_frame_target_types(const SchemaFrame &frame)
    -> std::unordered_map<std::string_view, SchemaFrameTargetType> {
  std::unordered_map<std::string_view, SchemaFrameTargetType> result;
  frame.for_each_reference(
      [&](const SchemaReferenceType, const WeakPointer &origin,
          const SchemaFrame::Reference &reference) -> void {
        if (schema_is_metaschema_reference(origin)) {
          return;
        }

        // A reference is always a keyword of the subschema that declares it, so
        // that subschema is what the origin sits directly inside of
        const auto reference_location{frame.traverse(origin.initial())};
        assert(reference_location.has_value());
        auto &target_type{result[reference.destination]};
        if (reference_location->get().property_name) {
          target_type.property_name = true;
        } else {
          target_type.instance = true;
        }
      });

  std::unordered_map<std::string_view, const WeakPointer *>
      destination_pointers;
  for (const auto &[destination, target_type] : result) {
    const auto destination_location{frame.traverse(destination)};
    if (destination_location.has_value()) {
      destination_pointers.emplace(destination,
                                   &destination_location->get().pointer);
    }
  }

  std::unordered_map<std::string_view, std::vector<std::string_view>>
      references_within;
  frame.for_each_reference(
      [&](const SchemaReferenceType, const WeakPointer &origin,
          const SchemaFrame::Reference &reference) -> void {
        if (schema_is_metaschema_reference(origin)) {
          return;
        }

        for (const auto &[destination, destination_pointer] :
             destination_pointers) {
          if (origin.starts_with(*destination_pointer) &&
              origin.size() > destination_pointer->size()) {
            references_within[destination].push_back(reference.destination);
          }
        }
      });

  bool changed{true};
  while (changed) {
    changed = false;
    for (const auto &[destination, target_type] : result) {
      if (!target_type.property_name) {
        continue;
      }

      const auto iterator{references_within.find(destination)};
      if (iterator == references_within.end()) {
        continue;
      }

      for (const auto &referenced_destination : iterator->second) {
        // Every destination that reaches here was seeded by the pass above,
        // which is why this reads rather than inserts. Inserting could rehash
        // the map that the enclosing loop is walking
        auto &referenced_target_type{result.at(referenced_destination)};
        if (!referenced_target_type.property_name) {
          referenced_target_type.property_name = true;
          changed = true;
        }
      }
    }
  }

  return result;
}

// Waiting on: `SchemaFrame`, as members of it, each of which it is better
// placed to answer than any caller is

// The pointer into the analysed document that a pointer relative to the given
// base addresses, which is the inverse of the URI that a frame already hands
// back for a location. A base that the frame locates no resource for is the one
// the caller framed a wrapper document with, under which every relative pointer
// starts at the top of the document
inline auto schema_frame_absolute_pointer(const SchemaFrame &frame,
                                          const std::string_view base,
                                          const WeakPointer &relative)
    -> WeakPointer {
  const auto resource{frame.location(SchemaReferenceType::Static, base)};
  if (!resource.has_value()) {
    // Framing holds the base it was given, so once this is a member the check
    // is a comparison rather than this scan. Missing the resource of any other
    // base would silently address a different place, so catch that drift here
    assert(frame.any_subschema(
        [base](const SchemaFrame::Location &location) -> bool {
          return location.base == base && location.relative_pointer == 0;
        }));
    return relative;
  }

  return resource.value().get().pointer.concat(relative);
}

// Whether any schema resource or subschema that evaluation can reach from the
// given entry point satisfies the predicate. A frame offers both halves of this
// on its own, and the order they go in matters, as the reachability of a
// location costs a walk while a predicate tends not to
template <std::predicate<const SchemaFrame::Location &> F>
auto schema_frame_any_reachable_subschema(
    const SchemaFrame &frame, const SchemaFrame::Location &entrypoint,
    const SchemaWalker &walker, const SchemaResolver &resolver,
    const F &predicate) -> bool {
  return frame.any_subschema(
      [&](const SchemaFrame::Location &location) -> bool {
        return predicate(location) &&
               frame.is_reachable(entrypoint, location, walker, resolver);
      });
}

// Whether anything strictly below this pointer is a schema resource or a
// subschema of its own, as the named counterpart of the questions a frame
// already answers about the references that reach a pointer
inline auto schema_frame_has_subschemas_under(const SchemaFrame &frame,
                                              const WeakPointer &pointer)
    -> bool {
  return frame.any_subschema_under(
      pointer, [](const SchemaFrame::Location &) -> bool { return true; });
}

} // namespace sourcemeta::core

#endif
