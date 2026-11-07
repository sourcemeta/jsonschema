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

// The parts of a subschema that apply to the data, and what they look at:
// the properties they name, and the items they look at.

namespace {

using sourcemeta::blaze::defines;
using sourcemeta::blaze::DocumentationContext;
using sourcemeta::blaze::dynamic_landings;
using sourcemeta::blaze::landing_of;
using sourcemeta::blaze::Rank;
using sourcemeta::blaze::rank_at;
using sourcemeta::blaze::Reach;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

// A part of a subschema that applies to the same data in place, such as a
// member of `allOf` or where a reference lands. It applies always, or only
// when a branch or a condition happens to match.
struct Part {
  Pointer pointer;
  bool always;
};

// The subschemas a keyword that holds a list of them, or a single one, applies.
auto add_members(std::vector<Part> &result, const JSON &subschema,
                 const Pointer &pointer, const std::string &keyword,
                 const bool always) -> void {
  if (!defines(subschema, keyword)) {
    return;
  }

  const auto &list{subschema.at(keyword)};
  if (list.is_array()) {
    for (std::size_t index = 0; index < list.array_size(); index++) {
      if (list.at(index).is_object()) {
        result.push_back({.pointer = pointer.concat(keyword).concat(index),
                          .always = always});
      }
    }
  } else if (list.is_object() && keyword != "anyOf" && keyword != "oneOf" &&
             keyword != "allOf") {
    result.push_back({.pointer = pointer.concat(keyword), .always = always});
  }
}

auto parts_of(const DocumentationContext &context, const JSON &subschema,
              const Pointer &pointer, const Rank rank) -> std::vector<Part> {
  std::vector<Part> result;
  if (!subschema.is_object()) {
    return result;
  }

  if (defines(subschema, "$ref") && subschema.at("$ref").is_string()) {
    std::string destination;
    const auto target{landing_of(context, pointer, "$ref", destination)};
    if (target.has_value()) {
      result.push_back({.pointer = target.value(), .always = true});
    }

    // Before 2019-09, a reference stands alone.
    if (rank < Rank::Draft2019) {
      return result;
    }
  }

  for (const std::string keyword : {"$dynamicRef", "$recursiveRef"}) {
    if (!defines(subschema, keyword) || !subschema.at(keyword).is_string() ||
        (keyword == "$dynamicRef" && rank < Rank::Draft2020) ||
        (keyword == "$recursiveRef" && rank != Rank::Draft2019)) {
      continue;
    }

    const auto targets{dynamic_landings(context, pointer, keyword)};
    for (const auto &target : targets) {
      result.push_back({.pointer = target, .always = targets.size() == 1});
    }
  }

  if (rank == Rank::Draft3) {
    add_members(result, subschema, pointer, "extends", true);
    add_members(result, subschema, pointer, "type", false);
    // A dependency that is a schema applies when its property is there.
    if (defines(subschema, "dependencies") &&
        subschema.at("dependencies").is_object()) {
      for (const auto &entry : subschema.at("dependencies").as_object()) {
        if (entry.second.is_object()) {
          result.push_back(
              {.pointer = pointer.concat("dependencies").concat(entry.first),
               .always = false});
        }
      }
    }

    return result;
  }

  add_members(result, subschema, pointer, "allOf", true);
  add_members(result, subschema, pointer, "anyOf", false);
  add_members(result, subschema, pointer, "oneOf", false);
  if (rank >= Rank::Draft7) {
    for (const std::string keyword : {"if", "then", "else"}) {
      if (defines(subschema, keyword) && subschema.at(keyword).is_object()) {
        result.push_back({.pointer = pointer.concat(keyword), .always = false});
      }
    }
  }

  for (const std::string dependencies : {"dependentSchemas", "dependencies"}) {
    if (dependencies == "dependentSchemas" && rank < Rank::Draft2019) {
      continue;
    }

    if (defines(subschema, dependencies) &&
        subschema.at(dependencies).is_object()) {
      for (const auto &entry : subschema.at(dependencies).as_object()) {
        if (entry.second.is_object()) {
          result.push_back(
              {.pointer = pointer.concat(dependencies).concat(entry.first),
               .always = false});
        }
      }
    }
  }

  return result;
}

auto gather_reach(const DocumentationContext &context, const JSON &subschema,
                  const Pointer &pointer, const Rank rank, const bool always,
                  std::set<std::string> &seen, Reach &reach) -> void {
  for (const auto &part : parts_of(context, subschema, pointer, rank)) {
    const auto applies{always && part.always};
    // The same place may be reached both always and only sometimes, and what
    // it says differs between the two.
    if (!seen.insert(sourcemeta::core::to_string(part.pointer) +
                     (applies ? "!" : "?"))
             .second) {
      continue;
    }

    const auto &member{sourcemeta::core::get(context.root, part.pointer)};
    if (!member.is_object()) {
      continue;
    }

    const auto member_rank{rank_at(context, part.pointer)};
    for (const auto *keyword : {"properties", "patternProperties"}) {
      if (defines(member, keyword) && member.at(keyword).is_object()) {
        for (const auto &entry : member.at(keyword).as_object()) {
          (std::string_view{keyword} == "properties" ? reach.names
                                                     : reach.patterns)
              .insert(entry.first);
        }
      }
    }

    if (applies && (defines(member, "additionalProperties") ||
                    (member_rank >= Rank::Draft2019 &&
                     defines(member, "unevaluatedProperties")))) {
      reach.every_property = true;
    }

    for (const auto *keyword :
         {"additionalProperties", "unevaluatedProperties"}) {
      if (defines(member, keyword) &&
          !(member.at(keyword).is_boolean() &&
            !member.at(keyword).to_boolean()) &&
          (std::string_view{keyword} == "additionalProperties" ||
           member_rank >= Rank::Draft2019)) {
        reach.names_every_property = true;
      }
    }

    // What this part looks at among the items.
    std::size_t prefix{0};
    auto every{false};
    auto looks{false};
    const auto *const list{member_rank >= Rank::Draft2020 ? "prefixItems"
                                                          : "items"};
    if (defines(member, list) && member.at(list).is_array()) {
      prefix = member.at(list).array_size();
      looks = true;
      if (member_rank < Rank::Draft2020 && defines(member, "additionalItems")) {
        every = true;
      }
    }

    if (defines(member, "items") &&
        (member_rank >= Rank::Draft2020 || !member.at("items").is_array())) {
      every = true;
    }

    if (member_rank >= Rank::Draft2019 && defines(member, "unevaluatedItems")) {
      every = true;
    }

    const auto contains{member_rank >= Rank::Draft2020 &&
                        defines(member, "contains")};
    looks = looks || every || contains;
    if (looks && !applies) {
      reach.items_sometimes = true;
    } else if (looks) {
      reach.prefix = std::max(reach.prefix, prefix);
      reach.every_item = reach.every_item || every;
      if (contains) {
        reach.contains.push_back(part.pointer.concat("contains"));
      }
    }

    gather_reach(context, member, part.pointer, member_rank, applies, seen,
                 reach);
  }
}

} // namespace

namespace sourcemeta::blaze {

auto reach_of(const DocumentationContext &context, const JSON &subschema,
              const Pointer &pointer) -> Reach {
  Reach reach;
  std::set<std::string> seen;
  gather_reach(context, subschema, pointer, context.rank, true, seen, reach);
  return reach;
}

} // namespace sourcemeta::blaze
