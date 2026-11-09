#ifndef SOURCEMETA_BLAZE_COMPILER_UPSTREAM_H_
#define SOURCEMETA_BLAZE_COMPILER_UPSTREAM_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::ranges::all_of, std::max
#include <cassert>       // assert
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

// JSON Schema describes `required` and its relatives as an array of unique
// property names. A value that does not satisfy that names no property at all,
// so nothing may be derived from it
inline auto schema_is_property_name_array(const JSON &value) -> bool {
  return value.is_array() && value.unique() &&
         std::ranges::all_of(value.as_array(), [](const auto &entry) -> bool {
           return entry.is_string();
         });
}

// Waiting on: the `jsonschema` module, where it lived until Core #980

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
// nothing that crosses between them

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

// Waiting on: `SchemaFrame`, as `SchemaFrame::TargetType` and a member that
// returns them

// A `$schema` reference points at a meta-schema rather than into the schema
// itself, so it describes the document rather than what the document says
inline auto schema_is_metaschema_reference(const WeakPointer &origin) -> bool {
  return !origin.empty() && origin.back().is_property() &&
         origin.back().to_property() == "$schema";
}

// What a reference target gets evaluated against. Both hold of a target that
// references of both kinds arrive at
struct SchemaFrameTargetType {
  bool property_name{false};
  bool instance{false};
};

// What every reference target in a frame gets evaluated against, keyed by
// destination. A target evaluated against property names passes that on to
// everything it references in turn, so the answer is a closure rather than a
// tally of the references that arrive directly
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

} // namespace sourcemeta::core

#endif
