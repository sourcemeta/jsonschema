#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::ranges::sort
#include <array>         // std::array, std::to_array
#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint16_t
#include <functional>    // std::ranges::greater, std::reference_wrapper
#include <limits>        // std::numeric_limits
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

// The order in which keywords are meant to appear within a schema, where the
// position of an entry is the rank of the keyword it names
constexpr auto KEYWORDS{std::to_array<std::string_view>(
    {// Most core keywords tend to come first
     "$schema", "$id", "id", "$vocabulary", "$anchor", "$dynamicAnchor",
     "$recursiveAnchor",

     // Then important metadata about the schema
     "title", "description", "$comment", "examples", "deprecated", "readOnly",
     "writeOnly", "default",

     // This is a placeholder for "x-"-prefixed unknown keywords, as they are
     // almost always metadata
     "x-",

     // Then references
     "$ref", "$dynamicRef", "$recursiveRef",

     // Then keywords that apply to any type
     "type", "disallow", "extends", "const", "enum", "optional", "requires",
     "allOf", "anyOf", "oneOf", "not", "if", "then", "else",

     // Then keywords about numbers
     "exclusiveMaximum", "maximum", "maximumCanEqual", "exclusiveMinimum",
     "minimum", "minimumCanEqual", "multipleOf", "divisibleBy", "maxDecimal",

     // Then keywords about strings
     "pattern", "format", "maxLength", "minLength", "contentEncoding",
     "contentMediaType", "contentSchema",

     // Then keywords about arrays
     "maxItems", "minItems", "uniqueItems", "maxContains", "minContains",
     "contains", "prefixItems", "items", "additionalItems", "unevaluatedItems",

     // Then keywords about objects
     "required", "maxProperties", "minProperties", "propertyNames",
     "properties", "patternProperties", "additionalProperties",
     "unevaluatedProperties", "dependentRequired", "dependencies",
     "dependentSchemas",

     // Reusable utilities go last
     "$defs", "definitions"})};

constexpr std::string_view EXTENSION_PREFIX{"x-"};

auto make_index() -> std::unordered_map<std::string_view, std::uint16_t> {
  std::unordered_map<std::string_view, std::uint16_t> index;
  index.reserve(KEYWORDS.size());
  for (std::size_t position = 0; position < KEYWORDS.size(); position += 1) {
    index.emplace(KEYWORDS[position], static_cast<std::uint16_t>(position));
  }

  return index;
}

// NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
const auto INDEX{make_index()};

auto keyword_rank(const JSON::String &keyword) -> std::uint16_t {
  constexpr auto UNRECOGNISED{std::numeric_limits<std::uint16_t>::max()};
  const auto match{INDEX.find(keyword.starts_with(EXTENSION_PREFIX)
                                  ? EXTENSION_PREFIX
                                  : std::string_view{keyword})};
  if (match == INDEX.cend()) {
    return UNRECOGNISED;
  }

  return match->second;
}

auto keyword_compare(const JSON::String &left, const JSON::String &right)
    -> bool {
  const auto left_rank{keyword_rank(left)};
  const auto right_rank{keyword_rank(right)};
  if (left_rank == right_rank) {
    return left < right;
  }

  return left_rank < right_rank;
}

} // namespace

auto schema_format(JSON &schema, const SchemaFrame &frame) -> void {
  assert(schema.is_object() || schema.is_boolean());

  std::vector<std::reference_wrapper<const WeakPointer>> subschemas;
  frame.for_each_subschema(
      [&subschemas](const SchemaFrame::Location &location) -> void {
        subschemas.emplace_back(location.pointer);
      });

  // Reordering an object moves its properties around, so a location that names
  // one of them stops standing for what it used to. Going from the deepest
  // subschema upwards means that by the time an object moves, every location
  // that goes through it was already taken care of
  std::ranges::sort(subschemas, std::ranges::greater{},
                    [](const auto &pointer) { return pointer.get().size(); });

  for (const auto &pointer : subschemas) {
    auto &subschema{get(schema, pointer.get())};
    if (subschema.is_object()) {
      subschema.reorder(keyword_compare);
    }
  }
}

} // namespace sourcemeta::core
