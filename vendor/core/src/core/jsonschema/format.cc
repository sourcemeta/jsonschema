#include <sourcemeta/core/jsonschema.h>

#include <cstdint>       // std::uint64_t
#include <limits>        // std::numeric_limits
#include <string>        // std::string
#include <unordered_map> // std::unordered_map

namespace {

auto keyword_rank(const sourcemeta::core::JSON::String &keyword,
                  const std::uint64_t otherwise) -> std::uint64_t {
  using Rank =
      std::unordered_map<sourcemeta::core::JSON::String, std::uint64_t>;
  static Rank rank{// Most core keywords tend to come first
                   {"$schema", 0},
                   {"$id", 1},
                   {"id", 2},
                   {"$vocabulary", 3},
                   {"$anchor", 4},
                   {"$dynamicAnchor", 5},
                   {"$recursiveAnchor", 6},

                   // Then important metadata about the schema
                   {"title", 7},
                   {"description", 8},
                   {"$comment", 10},
                   {"examples", 11},
                   {"deprecated", 12},
                   {"readOnly", 13},
                   {"writeOnly", 14},
                   {"default", 15},

                   // This is a placeholder for "x-"-prefixed unknown keywords,
                   // as they are almost always metadata
                   {"x", 16},

                   // Then references
                   {"$ref", 17},
                   {"$dynamicRef", 18},
                   {"$recursiveRef", 19},

                   // Then keywords that apply to any type
                   {"type", 20},
                   {"disallow", 21},
                   {"extends", 22},
                   {"const", 23},
                   {"enum", 24},
                   {"optional", 25},
                   {"requires", 26},
                   {"allOf", 27},
                   {"anyOf", 28},
                   {"oneOf", 29},
                   {"not", 30},
                   {"if", 31},
                   {"then", 32},
                   {"else", 33},

                   // Then keywords about numbers
                   {"exclusiveMaximum", 34},
                   {"maximum", 35},
                   {"maximumCanEqual", 36},
                   {"exclusiveMinimum", 37},
                   {"minimum", 38},
                   {"minimumCanEqual", 39},
                   {"multipleOf", 40},
                   {"divisibleBy", 41},
                   {"maxDecimal", 42},

                   // Then keywords about strings
                   {"pattern", 43},
                   {"format", 44},
                   {"maxLength", 45},
                   {"minLength", 46},
                   {"contentEncoding", 47},
                   {"contentMediaType", 48},
                   {"contentSchema", 49},

                   // Then keywords about arrays
                   {"maxItems", 50},
                   {"minItems", 51},
                   {"uniqueItems", 52},
                   {"maxContains", 53},
                   {"minContains", 54},
                   {"contains", 55},
                   {"prefixItems", 56},
                   {"items", 57},
                   {"additionalItems", 58},
                   {"unevaluatedItems", 59},

                   // Object
                   {"required", 60},
                   {"maxProperties", 61},
                   {"minProperties", 62},
                   {"propertyNames", 63},
                   {"properties", 64},
                   {"patternProperties", 65},
                   {"additionalProperties", 66},
                   {"unevaluatedProperties", 67},
                   {"dependentRequired", 68},
                   {"dependencies", 69},
                   {"dependentSchemas", 70},

                   // Reusable utilities go last
                   {"$defs", 71},
                   {"definitions", 72}};

  // A common pattern that seems to come up often in practice is schema authors
  // coming up with unknown annotation keywords that are meant to extend or
  // complement existing ones. For example, `title:en` for `title`, etc. By
  // checking the prefixes of a keyword, we can accomodate that pattern very
  // nicely by keeping them right besides the keywords they are supposed to
  // extend. For performance reasons, we only apply such logic to keywords
  // that have certain special characters that are commonly used for these kind
  // of extensions
  const auto pivot{keyword.find_first_of("-_:")};
  if (pivot != std::string::npos) {
    const auto match{rank.find(keyword.substr(0, pivot))};
    if (match != rank.cend()) {
      return match->second;
    }
  }

  const auto match{rank.find(keyword)};
  if (match != rank.cend()) {
    return match->second;
  } else {
    return otherwise;
  }
}

auto keyword_compare(const sourcemeta::core::JSON::String &left,
                     const sourcemeta::core::JSON::String &right) -> bool {
  constexpr auto DEFAULT{std::numeric_limits<std::uint64_t>::max()};
  const auto left_rank{keyword_rank(left, DEFAULT)};
  const auto right_rank{keyword_rank(right, DEFAULT)};
  if (left_rank == right_rank) {
    return left < right;
  } else {
    return left_rank < right_rank;
  }
}

} // namespace

namespace sourcemeta::core {

auto format(JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<JSON::String> &default_dialect) -> void {
  assert(is_schema(schema));
  SchemaFrame frame{SchemaFrame::Mode::Locations};
  frame.analyse(schema, walker, resolver, default_dialect);

  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Resource &&
        entry.second.type != SchemaFrame::LocationType::Subschema) {
      continue;
    }

    auto &value{get(schema, entry.second.pointer)};
    if (value.is_object()) {
      value.reorder(keyword_compare);
    }
  }
}

} // namespace sourcemeta::core
