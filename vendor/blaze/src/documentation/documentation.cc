#include <sourcemeta/blaze/documentation.h>

#include "internal.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>   // std::sort
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <exception>   // std::exception
#include <map>         // std::map
#include <optional>    // std::optional
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

// Walking a whole schema: deciding which dialect it is written in, naming the
// shapes it refers to, and assembling the document. Describing one subschema
// lives in `documentation_value.cc`.

namespace {

using sourcemeta::blaze::DocumentationContext;
using sourcemeta::blaze::fail;
using sourcemeta::blaze::MAXIMUM_DEPTH;
using sourcemeta::blaze::Rank;
using sourcemeta::core::JSON;
using sourcemeta::core::Pointer;

// How deeply the document nests, counted without recursion so that measuring a
// runaway schema cannot itself exhaust the stack.
auto nesting_of(const JSON &document) -> std::size_t {
  std::size_t deepest{0};
  std::vector<std::pair<const JSON *, std::size_t>> pending{{&document, 1}};
  while (!pending.empty()) {
    const auto [current, depth]{pending.back()};
    pending.pop_back();
    deepest = std::max(deepest, depth);
    if (depth > MAXIMUM_DEPTH * 2) {
      // No need to look further: it is already too deep to describe.
      return depth;
    }

    if (current->is_object()) {
      for (const auto &entry : current->as_object()) {
        pending.emplace_back(&entry.second, depth + 1);
      }
    } else if (current->is_array()) {
      for (const auto &entry : current->as_array()) {
        pending.emplace_back(&entry, depth + 1);
      }
    }
  }

  return deepest;
}

auto rank_of(const sourcemeta::core::SchemaBaseDialect base_dialect,
             const Pointer &pointer) -> Rank {
  // A HyperSchema adds links to the dialect it extends. Links say where to go
  // from the data rather than what the data may be, so the data is described
  // as in that dialect.
  switch (base_dialect) {
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
      return Rank::Draft2020;
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
      return Rank::Draft2019;
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
      return Rank::Draft7;
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
      return Rank::Draft6;
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
      return Rank::Draft4;
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
      return Rank::Draft3;
    default:
      fail("This module describes JSON Schema Draft 3 to 2020-12", pointer);
  }
}

auto name_of(const Pointer &pointer, const std::string &identifier)
    -> std::string {
  // The last part of the path that is a name of its own, skipping the KEYWORDS
  // and the positions a reader does not think in.
  static const std::vector<std::string> KEYWORDS{"properties",
                                                 "patternProperties",
                                                 "additionalProperties",
                                                 "items",
                                                 "prefixItems",
                                                 "additionalItems",
                                                 "contains",
                                                 "propertyNames",
                                                 "allOf",
                                                 "anyOf",
                                                 "oneOf",
                                                 "not",
                                                 "if",
                                                 "then",
                                                 "else",
                                                 "$defs",
                                                 "definitions",
                                                 "dependentSchemas",
                                                 "dependencies",
                                                 "unevaluatedItems",
                                                 "unevaluatedProperties"};
  std::string result{identifier};
  for (const auto &token : pointer) {
    if (!token.is_property()) {
      continue;
    }

    const auto &name{token.to_property()};
    if (std::find(KEYWORDS.cbegin(), KEYWORDS.cend(), name) ==
        KEYWORDS.cend()) {
      // The deepest such name is the one a reader knows the shape by.
      result = name;
    }
  }

  return result;
}

// The vocabularies the official meta-schema of a dialect uses. Only 2019-09
// and 2020-12 let a meta-schema choose its vocabularies.
auto official_vocabularies(const Rank rank)
    -> const sourcemeta::core::SchemaVocabularies & {
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  static const sourcemeta::core::SchemaVocabularies DRAFT_2019{
      {Known::JSON_SCHEMA_2019_09_CORE, true},
      {Known::JSON_SCHEMA_2019_09_APPLICATOR, true},
      {Known::JSON_SCHEMA_2019_09_VALIDATION, true},
      {Known::JSON_SCHEMA_2019_09_META_DATA, true},
      {Known::JSON_SCHEMA_2019_09_FORMAT, false},
      {Known::JSON_SCHEMA_2019_09_CONTENT, true}};
  static const sourcemeta::core::SchemaVocabularies DRAFT_2020{
      {Known::JSON_SCHEMA_2020_12_CORE, true},
      {Known::JSON_SCHEMA_2020_12_APPLICATOR, true},
      {Known::JSON_SCHEMA_2020_12_UNEVALUATED, true},
      {Known::JSON_SCHEMA_2020_12_VALIDATION, true},
      {Known::JSON_SCHEMA_2020_12_META_DATA, true},
      {Known::JSON_SCHEMA_2020_12_FORMAT_ANNOTATION, true},
      {Known::JSON_SCHEMA_2020_12_CONTENT, true}};
  return rank == Rank::Draft2019 ? DRAFT_2019 : DRAFT_2020;
}

// A keyword of the dialect whose vocabulary the meta-schema of a subschema
// leaves out means nothing there, so it is set aside.
auto ignore_left_out_vocabularies(
    DocumentationContext &context,
    const sourcemeta::core::SchemaFrame::Location &location,
    const Pointer &where, const Rank rank) -> void {
  if (rank < Rank::Draft2019) {
    return;
  }

  const auto &subschema{sourcemeta::core::get(context.root, where)};
  if (!subschema.is_object()) {
    return;
  }

  const auto &vocabularies{
      context.frame.vocabularies(location, context.resolver)};
  const auto &official{official_vocabularies(rank)};
  for (const auto &entry : subschema.as_object()) {
    if (context.walker(entry.first, vocabularies).type ==
            sourcemeta::core::SchemaKeywordType::Unknown &&
        context.walker(entry.first, official).type !=
            sourcemeta::core::SchemaKeywordType::Unknown) {
      context.ignored[sourcemeta::core::to_string(where)].insert(entry.first);
    }
  }
}

} // namespace

namespace sourcemeta::blaze {

auto to_documentation(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaWalker &walker,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const std::string_view default_dialect)
    -> sourcemeta::core::JSON {
  const Pointer root_pointer;
  assert(schema.is_object() || schema.is_boolean());

  const auto declares{schema.is_object() && schema.defines("$schema") &&
                      schema.at("$schema").is_string() &&
                      !schema.at("$schema").to_string().empty()};
  if (!declares && default_dialect.empty()) {
    fail("The schema does not say which dialect it is written in",
         root_pointer);
  }

  const std::string declared{declares ? schema.at("$schema").to_string()
                                      : std::string{default_dialect}};

  // Reading a schema this deep would run out of stack before it could say
  // anything useful, in this module or in what it calls.
  if (nesting_of(schema) > MAXIMUM_DEPTH * 2) {
    fail("This schema nests deeper than this module describes", root_pointer);
  }

  std::optional<sourcemeta::core::SchemaFrame> frame;
  try {
    frame.emplace(sourcemeta::core::SchemaFrame::Mode::References, schema,
                  walker, resolver, declared);
  } catch (const std::exception &error) {
    fail("The schema could not be read", root_pointer, error.what());
  }

  const auto root_location{frame->root_location()};
  if (!root_location.has_value()) {
    fail("The schema does not say which dialect it is written in",
         root_pointer);
  }

  DocumentationContext context{
      .root = schema,
      .walker = walker,
      .resolver = resolver,
      .frame = frame.value(),
      .rank = rank_of(root_location->get().base_dialect, root_pointer),
      .ranks = {},
      .shapes = {},
      .references = {},
      .dynamic_references = {},
      .dynamic_anchors = {},
      .ignored = {},
      .depth = 0};

  // A resource may declare a dialect of its own, and then a keyword means
  // something else, or nothing at all, from one part of the schema to the
  // next. `prefixItems` inside a Draft 7 resource is not a rule. So every
  // subschema is read in the dialect in effect where it sits.
  frame->for_each_subschema([&context](const auto &location) {
    auto where{sourcemeta::core::to_pointer(location.pointer)};
    const auto rank{rank_of(location.base_dialect, where)};
    context.ranks.insert_or_assign(sourcemeta::core::to_string(where), rank);
    ignore_left_out_vocabularies(context, location, where, rank);
  });

  frame->for_each_reference([&context](const auto type,
                                       const auto &reference_pointer,
                                       const auto &reference) {
    auto &into{type == sourcemeta::core::SchemaReferenceType::Static
                   ? context.references
                   : context.dynamic_references};
    into.emplace(sourcemeta::core::to_string(reference_pointer),
                 reference.destination);
  });

  frame->for_each_location([&context](const auto type, const auto &uri,
                                      const auto &location) {
    if (type != sourcemeta::core::SchemaReferenceType::Dynamic) {
      return;
    }

    const auto hash{uri.find('#')};
    const std::string name{hash == std::string_view::npos ? std::string_view{}
                                                          : uri.substr(hash)};
    auto where{sourcemeta::core::to_pointer(location.pointer)};

    // A resource nested in another is known by more than one address, so the
    // same place can turn up more than once.
    auto &places{context.dynamic_anchors[name]};
    if (std::find(places.cbegin(), places.cend(), where) == places.cend()) {
      places.push_back(std::move(where));
    }
  });

  auto root{describe_subschema(context, schema, root_pointer)};

  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("$schema",
                sourcemeta::core::JSON{std::string{DOCUMENTATION_FORMAT}});
  // The root's own title and description are the document's. A root that is a
  // reference before 2019-09 stands for the reference alone, yet its title
  // still names the document, unless its vocabulary is left out.
  const auto ignored{
      context.ignored.find(sourcemeta::core::to_string(root_pointer))};
  const auto title_ignored{ignored != context.ignored.cend() &&
                           ignored->second.contains("title")};
  if (root.defines("title")) {
    result.assign("title", root.at("title"));
  } else if (!title_ignored && schema.is_object() && schema.defines("title") &&
             schema.at("title").is_string() &&
             !schema.at("title").to_string().empty()) {
    result.assign("title", schema.at("title"));
  } else {
    result.assign("title", sourcemeta::core::JSON{std::string{"Schema"}});
  }
  if (root.defines("description")) {
    result.assign("description", root.at("description"));
  }

  result.assign("language", sourcemeta::core::JSON{std::string{declared}});
  // The root describes itself, so what it says is not repeated as a shape.
  root.erase(JSON::String{"title"});
  root.erase(JSON::String{"description"});
  result.assign("root", std::move(root));

  // Every place a reference lands is described once and linked to.
  // Converting a shape can find more shapes, at any depth, so this keeps
  // going until nothing new turns up.
  std::map<std::string, sourcemeta::core::JSON> values;
  while (true) {
    std::vector<Shape> pending;
    for (const auto &entry : context.shapes) {
      if (entry.second.pointer.empty() || values.contains(entry.first)) {
        continue;
      }

      pending.push_back(entry.second);
    }

    if (pending.empty()) {
      break;
    }

    for (const auto &shape : pending) {
      values.emplace(shape.identifier,
                     describe_subschema(
                         context, sourcemeta::core::get(schema, shape.pointer),
                         shape.pointer));
    }
  }

  std::vector<Shape> shapes;
  for (const auto &entry : context.shapes) {
    if (entry.second.pointer.empty()) {
      continue;
    }

    shapes.push_back(entry.second);
  }

  // Most used first, so a reader meets the common ones early.
  std::sort(shapes.begin(), shapes.end(),
            [](const auto &left, const auto &right) {
              if (left.uses != right.uses) {
                return left.uses > right.uses;
              }

              return left.identifier < right.identifier;
            });

  if (!shapes.empty()) {
    auto entries{sourcemeta::core::JSON::make_array()};
    for (const auto &shape : shapes) {
      auto entry{sourcemeta::core::JSON::make_object()};
      entry.assign("id", sourcemeta::core::JSON{shape.identifier});
      entry.assign("name", sourcemeta::core::JSON{
                               name_of(shape.pointer, shape.identifier)});
      entry.assign("value", values.at(shape.identifier));
      entries.push_back(std::move(entry));
    }

    result.assign("shapes", std::move(entries));
  }

  return result;
}

} // namespace sourcemeta::blaze
