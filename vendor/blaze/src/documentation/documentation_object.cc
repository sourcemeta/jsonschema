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

// What an object holds: its properties, which of them are required, what
// the properties it does not name hold, and what follows from filling one in.

namespace {

using sourcemeta::blaze::make_value;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

// An object that only says a property must be present, for the `when` of a
// condition made from a dependency keyword.
auto requires_property(const JSON::String &name) -> JSON {
  auto property{JSON::make_object()};
  property.assign("name", JSON{name});
  property.assign("required", JSON{true});
  auto properties{JSON::make_array()};
  properties.push_back(std::move(property));
  auto object{make_value("object")};
  object.assign("properties", std::move(properties));
  return object;
}

auto requires_properties(const JSON &names) -> JSON {
  assert(names.is_array());

  if (names.empty()) {
    // Nothing becomes necessary, so there is nothing to say.
    return make_value("any");
  }

  auto properties{JSON::make_array()};
  for (const auto &name : names.as_array()) {
    assert(name.is_string());

    auto property{JSON::make_object()};
    property.assign("name", name);
    property.assign("required", JSON{true});
    properties.push_back(std::move(property));
  }

  auto object{make_value("object")};
  object.assign("properties", std::move(properties));
  return object;
}

// Whether a key rule lets a name through. What it cannot decide, it allows,
// since the table may only state what is certain.
auto name_allowed(const JSON &keys, const JSON::String &name) -> bool {
  if (!keys.is_object()) {
    return true;
  }

  if (keys.at("kind").to_string() == "never") {
    return false;
  }

  if (keys.at("kind").to_string() == "enum" && keys.defines("values")) {
    for (const auto &allowed : keys.at("values").as_array()) {
      if (allowed.is_string() && allowed.to_string() == name) {
        return true;
      }
    }

    return false;
  }

  if (keys.defines("pattern") && keys.at("pattern").is_string() &&
      !sourcemeta::core::matches_if_valid(keys.at("pattern").to_string(),
                                          name)) {
    return false;
  }

  if (keys.defines("length")) {
    const auto &length{keys.at("length")};
    // Counted the way the specification counts, in characters.
    const JSON size{
        static_cast<std::int64_t>(sourcemeta::core::JSON::size(name))};
    if (length.defines("min") && length.at("min").is_number() &&
        size < length.at("min")) {
      return false;
    }

    if (length.defines("max") && length.at("max").is_number() &&
        length.at("max") < size) {
      return false;
    }
  }

  return true;
}

// Whether a name matches one of the given patterns.
auto matches_any(const std::vector<JSON::String> &patterns,
                 const JSON::String &name) -> bool {
  return std::any_of(patterns.cbegin(), patterns.cend(),
                     [&name](const auto &pattern) {
                       return sourcemeta::core::matches_if_valid(pattern, name);
                     });
}

// The names, and the patterns of names, a subschema names itself.
auto exceptions(const std::set<JSON::String> &names,
                const std::vector<JSON::String> &patterns) -> JSON {
  auto result{JSON::make_object()};
  if (!names.empty()) {
    auto listed{JSON::make_array()};
    for (const auto &name : names) {
      listed.push_back(JSON{name});
    }

    result.assign("names", std::move(listed));
  }

  if (!patterns.empty()) {
    auto listed{JSON::make_array()};
    for (const auto &pattern : patterns) {
      listed.push_back(JSON{pattern});
    }

    result.assign("patterns", std::move(listed));
  }

  return result;
}

// What the properties matching a pattern, or every property when there is no
// pattern, hold, apart from the exceptions.
auto held_by(const JSON::String *pattern, const JSON &except, const JSON &value)
    -> JSON {
  auto entry{JSON::make_object()};
  if (pattern != nullptr) {
    entry.assign("pattern", JSON{*pattern});
  }

  if (!except.empty()) {
    entry.assign("except", except);
  }

  entry.assign("value", value);
  return entry;
}

// Whether an entry of `patternProperties`, as the format writes it, is about a
// given name: one its pattern matches, or any name when it has none, and not
// one of its exceptions.
auto entry_covers(const JSON &entry, const JSON::String &name) -> bool {
  if (entry.defines("pattern") && !sourcemeta::core::matches_if_valid(
                                      entry.at("pattern").to_string(), name)) {
    return false;
  }

  if (!entry.defines("except")) {
    return true;
  }

  const auto &except{entry.at("except")};
  if (except.defines("names")) {
    for (const auto &excluded : except.at("names").as_array()) {
      if (excluded.to_string() == name) {
        return false;
      }
    }
  }

  if (except.defines("patterns")) {
    for (const auto &excluded : except.at("patterns").as_array()) {
      if (sourcemeta::core::matches_if_valid(excluded.to_string(), name)) {
        return false;
      }
    }
  }

  return true;
}

// The row for a property, made the first time the property is named, so that
// its name, whether it is required, and what it holds stay in that order.
auto property_named(std::vector<JSON> &properties,
                    std::map<JSON::String, std::size_t> &position,
                    const std::set<JSON::String> &required,
                    const JSON::String &name) -> JSON & {
  const auto match{position.find(name)};
  if (match != position.cend()) {
    return properties.at(match->second);
  }

  auto property{JSON::make_object()};
  property.assign("name", JSON{name});
  if (required.contains(name)) {
    property.assign("required", JSON{true});
  }

  position.emplace(name, properties.size());
  properties.push_back(std::move(property));
  return properties.back();
}

} // namespace

namespace sourcemeta::blaze {

auto object_facts(DocumentationContext &context, JSON &value,
                  const JSON &subschema, const Pointer &pointer) -> void {
  std::set<JSON::String> required;
  if (defines(subschema, "required") && context.rank >= Rank::Draft4) {
    const auto &names{subschema.at("required")};
    assert(names.is_array());

    for (const auto &name : names.as_array()) {
      assert(name.is_string());

      [[maybe_unused]] const auto unique{
          required.insert(name.to_string()).second};
      assert(unique);
    }
  }

  // Draft 3 says a property is required on the property itself.
  if (defines(subschema, "properties") && context.rank == Rank::Draft3 &&
      subschema.at("properties").is_object()) {
    for (const auto &entry : subschema.at("properties").as_object()) {
      if (defines(entry.second, "required") &&
          entry.second.at("required").is_boolean() &&
          entry.second.at("required").to_boolean()) {
        required.insert(entry.first);
      }
    }
  }

  std::vector<JSON> properties;
  std::map<JSON::String, std::size_t> position;

  std::set<JSON::String> own;
  if (defines(subschema, "properties")) {
    const auto &described{subschema.at("properties")};
    assert(described.is_object());

    for (const auto &entry : described.as_object()) {
      property_named(properties, position, required, entry.first)
          .assign("value",
                  describe_subschema(
                      context, entry.second,
                      pointer.concat("properties").concat(entry.first)));
      own.insert(entry.first);
    }
  }

  // A name that is required and described nowhere is still a property a reader
  // must know about.
  for (const auto &name : required) {
    property_named(properties, position, required, name);
  }

  std::vector<JSON::String> own_patterns;
  auto pattern_properties{JSON::make_array()};
  if (defines(subschema, "patternProperties")) {
    const auto &patterns{subschema.at("patternProperties")};
    assert(patterns.is_object());

    for (const auto &entry : patterns.as_object()) {
      auto pattern{JSON::make_object()};
      pattern.assign("pattern", JSON{entry.first});
      pattern.assign(
          "value",
          describe_subschema(
              context, entry.second,
              pointer.concat("patternProperties").concat(entry.first)));
      own_patterns.push_back(entry.first);
      pattern_properties.push_back(std::move(pattern));
    }
  }

  // The names that make other properties necessary, or bring in a schema.
  std::set<JSON::String> triggers;
  for (const auto *keyword :
       {"dependentRequired", "dependentSchemas", "dependencies"}) {
    if (defines(subschema, keyword) && subschema.at(keyword).is_object()) {
      for (const auto &entry : subschema.at(keyword).as_object()) {
        triggers.insert(entry.first);
      }
    }
  }

  // What every property that is not named holds, if anything is said.
  std::optional<JSON> other_properties;
  auto closed{false};
  if (defines(subschema, "additionalProperties")) {
    const auto &additional{subschema.at("additionalProperties")};
    if (additional.is_boolean() && additional.to_boolean()) {
      other_properties = make_value("any");
    } else {
      const auto other{
          additional.is_boolean()
              ? make_value("never")
              : describe_subschema(context, additional,
                                   pointer.concat("additionalProperties"))};
      // This keyword only counts the properties this subschema itself names.
      // A name that only a member of `allOf`, a branch, a condition or a
      // reference uses is held to it as well, and the format would otherwise
      // count that name as named.
      const auto reach{reach_of(context, subschema, pointer)};

      std::set<JSON::String> held{reach.names};
      held.insert(required.cbegin(), required.cend());
      held.insert(triggers.cbegin(), triggers.cend());
      for (const auto &name : held) {
        if (own.contains(name) || matches_any(own_patterns, name)) {
          continue;
        }

        auto &property{property_named(properties, position, required, name)};
        if (property.defines("value")) {
          also(property.at("value"), JSON{other});
        } else {
          property.assign("value", other);
        }
      }

      // A name this subschema names itself, or matches by a pattern of its
      // own, is not held to it, so what is held leaves those names out.
      const auto except{exceptions(own, own_patterns)};
      for (const auto &pattern : reach.patterns) {
        if (std::find(own_patterns.cbegin(), own_patterns.cend(), pattern) ==
            own_patterns.cend()) {
          pattern_properties.push_back(held_by(&pattern, except, other));
        }
      }

      // A part that says what every property it does not name holds names
      // every property, so the format would count them all as named. Every
      // name this subschema does not name itself is held here outright.
      if (reach.names_every_property) {
        pattern_properties.push_back(held_by(nullptr, except, other));
      }

      if (additional.is_boolean()) {
        closed = true;
      } else {
        other_properties = other;
      }
    }
  } else if (defines(subschema, "unevaluatedProperties") &&
             context.rank >= Rank::Draft2019) {
    // A property counts as looked at when a part of the subschema that applies
    // to the data names it, which is what the format means by a named
    // property. So this keyword says just what `closed` and `otherProperties`
    // say, unless a part that always applies already looks at every property.
    const auto &unevaluated{subschema.at("unevaluatedProperties")};
    const auto reach{reach_of(context, subschema, pointer)};
    if (!reach.every_property &&
        !(unevaluated.is_boolean() && unevaluated.to_boolean())) {
      if (unevaluated.is_boolean()) {
        closed = true;
      } else {
        other_properties = describe_subschema(
            context, unevaluated, pointer.concat("unevaluatedProperties"));
      }
    }
  }

  if (!properties.empty()) {
    auto entries{JSON::make_array()};
    for (auto &property : properties) {
      entries.push_back(std::move(property));
    }

    value.assign("properties", std::move(entries));
  }

  if (!pattern_properties.empty()) {
    value.assign("patternProperties", std::move(pattern_properties));
  }

  if (closed) {
    value.assign("closed", JSON{true});
  } else if (other_properties.has_value()) {
    value.assign("otherProperties", std::move(other_properties).value());
  }

  if (defines(subschema, "propertyNames") && context.rank >= Rank::Draft6) {
    value.assign("keys",
                 describe_subschema(context, subschema.at("propertyNames"),
                                    pointer.concat("propertyNames")));
  }

  // A property whose name also matches a pattern is held to both, so the table
  // says both of them rather than only the first.
  if (value.defines("properties") && value.defines("patternProperties")) {
    for (auto &property : value.at("properties").as_array()) {
      if (!property.defines("value")) {
        continue;
      }

      for (const auto &entry : value.at("patternProperties").as_array()) {
        if (!entry_covers(entry, property.at("name").to_string())) {
          continue;
        }

        also(property.at("value"), JSON{entry.at("value")});
      }
    }
  }

  // A name the key rule does not allow can never appear, whatever a property
  // says it would hold.
  if (value.defines("properties") && value.defines("keys")) {
    const auto &keys{value.at("keys")};
    for (auto &property : value.at("properties").as_array()) {
      if (!name_allowed(keys, property.at("name").to_string())) {
        property.assign("value", make_value("never"));
      }
    }
  }

  if (context.rank >= Rank::Draft4) {
    const auto count{bounds(subschema, "minProperties", "maxProperties")};
    if (count.has_value()) {
      value.assign("propertyCount", JSON{count.value()});
    }
  }

  // One property being present making others necessary is a condition.
  if (defines(subschema, "dependentRequired") &&
      context.rank >= Rank::Draft2019) {
    const auto &dependencies{subschema.at("dependentRequired")};
    assert(dependencies.is_object());

    for (const auto &entry : dependencies.as_object()) {
      auto entry_condition{JSON::make_object()};
      entry_condition.assign("when", requires_property(entry.first));
      entry_condition.assign("then", requires_properties(entry.second));
      condition(value, std::move(entry_condition));
    }
  }

  if (defines(subschema, "dependentSchemas") &&
      context.rank >= Rank::Draft2019) {
    const auto &dependencies{subschema.at("dependentSchemas")};
    assert(dependencies.is_object());

    for (const auto &entry : dependencies.as_object()) {
      auto entry_condition{JSON::make_object()};
      entry_condition.assign("when", requires_property(entry.first));
      entry_condition.assign(
          "then", describe_subschema(
                      context, entry.second,
                      pointer.concat("dependentSchemas").concat(entry.first)));
      condition(value, std::move(entry_condition));
    }
  }

  // Before 2019-09, both of the above were one keyword. Later dialects drop it,
  // but Blaze still holds data to it for the sake of older schemas, so it is
  // described wherever it is written.
  if (defines(subschema, "dependencies")) {
    const auto &dependencies{subschema.at("dependencies")};
    assert(dependencies.is_object());

    for (const auto &entry : dependencies.as_object()) {
      auto entry_condition{JSON::make_object()};
      entry_condition.assign("when", requires_property(entry.first));
      if (entry.second.is_array()) {
        entry_condition.assign("then", requires_properties(entry.second));
      } else if (entry.second.is_string() && context.rank == Rank::Draft3) {
        // Draft 3 may name the one property it makes necessary on its own.
        auto names{JSON::make_array()};
        names.push_back(entry.second);
        entry_condition.assign("then", requires_properties(names));
      } else {
        entry_condition.assign(
            "then", describe_subschema(
                        context, entry.second,
                        pointer.concat("dependencies").concat(entry.first)));
      }

      condition(value, std::move(entry_condition));
    }
  }
}

} // namespace sourcemeta::blaze
