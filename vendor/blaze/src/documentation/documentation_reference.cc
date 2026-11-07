#include "internal.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/regex.h>

#include <algorithm>   // std::sort, std::any_of, std::find, std::max
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t, std::uint8_t, std::uint32_t
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

// Where a reference lands: the place in the schema it names, or the places
// a dynamic reference may land on, and the shapes they become.

namespace {

using sourcemeta::blaze::defines;
using sourcemeta::blaze::DocumentationContext;
using sourcemeta::blaze::make_value;
using sourcemeta::blaze::Shape;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

// The schema resource a place belongs to: the nearest subschema around it that
// declares an identifier, or the root.
auto resource_of(const DocumentationContext &context, const Pointer &pointer)
    -> Pointer {
  Pointer current{pointer};
  while (!current.empty()) {
    if (context.ranks.contains(sourcemeta::core::to_string(current))) {
      const auto &candidate{sourcemeta::core::get(context.root, current)};
      if (defines(candidate, "$id") && candidate.at("$id").is_string()) {
        return current;
      }
    }

    current.pop_back();
  }

  return current;
}

// A place in this schema, described once as a shape and linked to wherever it
// is used.
auto shape_at(DocumentationContext &context, const Pointer &target) -> JSON {
  const auto identifier{sourcemeta::core::to_string(target)};
  auto match{context.shapes.find(identifier)};
  if (match == context.shapes.cend()) {
    match = context.shapes
                .emplace(identifier, Shape{.identifier = identifier,
                                           .pointer = target,
                                           .uses = 0})
                .first;
  }

  match->second.uses += 1;
  auto value{make_value("ref")};
  value.assign("ref", JSON{identifier});
  return value;
}

} // namespace

namespace sourcemeta::blaze {

// The dialect in effect at a place: the one of the subschema there, or else of
// the nearest one around it.
auto rank_at(const DocumentationContext &context, const Pointer &pointer)
    -> Rank {
  Pointer current{pointer};
  while (true) {
    const auto match{context.ranks.find(sourcemeta::core::to_string(current))};
    if (match != context.ranks.cend()) {
      return match->second;
    }

    if (current.empty()) {
      return context.rank;
    }

    current.pop_back();
  }
}

// Where a reference keyword of a subschema lands in this schema. Nothing means
// it leads to another document, kept at the address it gives.
auto landing_of(const DocumentationContext &context, const Pointer &pointer,
                const std::string &keyword, std::string &destination)
    -> std::optional<Pointer> {
  const auto entry{context.references.find(
      sourcemeta::core::to_string(pointer.concat(keyword)))};
  if (entry == context.references.cend()) {
    fail("This keyword names a place that does not resolve", pointer, keyword);
  }

  destination = entry->second;
  const auto target{context.frame.traverse(destination)};
  if (!target.has_value()) {
    // A reference into this document that lands nowhere is a schema that
    // cannot be read, rather than a document kept elsewhere.
    const auto &root{context.frame.root()};
    if (destination.starts_with('#') ||
        (!root.empty() && destination.starts_with(root))) {
      fail("This keyword names a place in this schema that does not resolve",
           pointer, keyword);
    }

    return std::nullopt;
  }

  const auto &location{target->get()};
  if (location.type != sourcemeta::core::SchemaFrame::LocationType::Resource &&
      location.type != sourcemeta::core::SchemaFrame::LocationType::Subschema &&
      location.type != sourcemeta::core::SchemaFrame::LocationType::Anchor &&
      location.type != sourcemeta::core::SchemaFrame::LocationType::Pointer) {
    fail("This keyword names something that is not a schema", pointer, keyword);
  }

  auto result{sourcemeta::core::to_pointer(location.pointer)};
  // A place that only holds subschemas, such as `properties`, or holds the
  // author's values, such as `enum`, is not a schema even when it reads as an
  // object.
  if (location.type == sourcemeta::core::SchemaFrame::LocationType::Pointer) {
    static const std::set<std::string> HOLDERS{"properties",
                                               "patternProperties",
                                               "$defs",
                                               "definitions",
                                               "dependentSchemas",
                                               "dependencies",
                                               "allOf",
                                               "anyOf",
                                               "oneOf",
                                               "prefixItems",
                                               "items",
                                               "extends",
                                               "type",
                                               "disallow",
                                               "enum",
                                               "const",
                                               "examples",
                                               "default",
                                               "required"};
    Pointer prefix;
    for (const auto &token : result) {
      if (token.is_property() && HOLDERS.contains(token.to_property()) &&
          context.ranks.contains(sourcemeta::core::to_string(prefix))) {
        const auto &holder{sourcemeta::core::get(
            context.root, prefix.concat(token.to_property()))};
        const auto container{holder.is_object() || holder.is_array()};
        const auto value_keyword{token.to_property() == "enum" ||
                                 token.to_property() == "const" ||
                                 token.to_property() == "examples" ||
                                 token.to_property() == "default" ||
                                 token.to_property() == "required"};
        // The holder itself, or anything inside an author's value.
        if ((container && prefix.size() + 1 == result.size()) ||
            value_keyword) {
          fail("This keyword names something that is not a schema", pointer,
               keyword);
        }
      }

      if (token.is_property()) {
        prefix.push_back(token.to_property());
      } else {
        prefix.push_back(token.to_index());
      }
    }
  }

  const auto &target_schema{sourcemeta::core::get(context.root, result)};
  if (!target_schema.is_object() && !target_schema.is_boolean()) {
    fail("This keyword names something that is not a schema", pointer, keyword);
  }

  return result;
}

// Where a `$dynamicRef` or `$recursiveRef` may land. Evaluation always starts
// at the root, so an anchor the root resource declares always wins. Otherwise
// the place depends on the path taken to get there, and each place it may be
// is one of the forms the value takes.
auto dynamic_landings(const DocumentationContext &context,
                      const Pointer &pointer, const std::string &keyword)
    -> std::vector<Pointer> {
  const auto entry{context.dynamic_references.find(
      sourcemeta::core::to_string(pointer.concat(keyword)))};
  if (entry == context.dynamic_references.cend()) {
    // It behaves as a plain reference, which the frame resolved as one.
    std::string destination;
    const auto target{landing_of(context, pointer, keyword, destination)};
    if (!target.has_value()) {
      return {};
    }

    return {target.value()};
  }

  const auto hash{entry->second.find('#')};
  const std::string name{keyword == "$recursiveRef" || hash == std::string::npos
                             ? std::string{}
                             : entry->second.substr(hash)};
  const auto places{context.dynamic_anchors.find(name)};
  if (places == context.dynamic_anchors.cend() || places->second.empty()) {
    // Nothing here declares it, so it leads to another document, unless it
    // was meant for this one.
    const auto &root{context.frame.root()};
    if (entry->second.starts_with('#') ||
        (!root.empty() && entry->second.starts_with(root))) {
      fail("This keyword names a place that does not resolve", pointer,
           keyword);
    }

    return {};
  }

  for (const auto &place : places->second) {
    if (resource_of(context, place).empty()) {
      return {place};
    }
  }

  return places->second;
}

// What the reference keywords of a subschema lead to, one value for each.
auto references_of(DocumentationContext &context, const JSON &subschema,
                   const Pointer &pointer) -> std::vector<JSON> {
  std::vector<JSON> result;
  for (const std::string keyword : {"$ref", "$dynamicRef", "$recursiveRef"}) {
    if (!defines(subschema, keyword) ||
        (keyword == "$dynamicRef" && context.rank < Rank::Draft2020) ||
        (keyword == "$recursiveRef" && context.rank != Rank::Draft2019)) {
      continue;
    }

    assert(subschema.at(keyword).is_string());

    // A dynamic reference whose target cannot change is a plain one.
    if (keyword == "$ref" ||
        !context.dynamic_references.contains(
            sourcemeta::core::to_string(pointer.concat(keyword)))) {
      std::string destination;
      const auto target{landing_of(context, pointer, keyword, destination)};
      if (target.has_value()) {
        result.push_back(shape_at(context, target.value()));
      } else {
        // Something outside this document, which a reader follows elsewhere.
        auto value{make_value("external")};
        value.assign("href", JSON{destination});
        result.push_back(std::move(value));
      }

      continue;
    }

    const auto targets{dynamic_landings(context, pointer, keyword)};
    if (targets.empty()) {
      // Something outside this document, which a reader follows elsewhere.
      auto value{make_value("external")};
      value.assign("href",
                   JSON{context.dynamic_references.at(
                       sourcemeta::core::to_string(pointer.concat(keyword)))});
      result.push_back(std::move(value));
    } else if (targets.size() == 1) {
      result.push_back(shape_at(context, targets.front()));
    } else if (!targets.empty()) {
      auto options{JSON::make_array()};
      for (const auto &target : targets) {
        options.push_back(shape_at(context, target));
      }

      auto choice{make_value("choice")};
      choice.assign("options", std::move(options));
      choice.assign("overlap", JSON{true});
      result.push_back(std::move(choice));
    }
  }

  return result;
}

} // namespace sourcemeta::blaze
