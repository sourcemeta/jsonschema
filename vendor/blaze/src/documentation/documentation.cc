#include <sourcemeta/blaze/documentation.h>

#include <sourcemeta/blaze/alterschema.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cassert> // assert
#include <cstdint> // std::int64_t
#include <map>     // std::map
#include <set>     // std::set
#include <sstream> // std::ostringstream
#include <string>  // std::to_string
#include <utility> // std::move

namespace sourcemeta::blaze {

namespace {

auto resolve_destination(const sourcemeta::core::JSON::String &raw_ref,
                         const sourcemeta::core::SchemaFrame &frame)
    -> std::optional<
        std::reference_wrapper<const sourcemeta::core::SchemaFrame::Location>> {
  auto result{frame.traverse(raw_ref)};
  if (result.has_value()) {
    return result;
  }
  for (const auto &[key, entry] : frame.references()) {
    if (key.first == sourcemeta::core::SchemaReferenceType::Static &&
        entry.original == raw_ref) {
      return frame.traverse(entry.destination);
    }
  }
  return std::nullopt;
}

struct VisitedEntry {
  std::size_t identifier;
  sourcemeta::core::JSON path;
};
using VisitedSchemas = std::map<const sourcemeta::core::JSON *, VisitedEntry>;
using RefChain = std::set<const sourcemeta::core::JSON *>;

auto type_expression_of(const sourcemeta::core::JSON &schema,
                        const sourcemeta::core::SchemaFrame &frame,
                        const sourcemeta::core::JSON &root,
                        const VisitedSchemas &visited, RefChain &ref_chain)
    -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};

  if (schema.is_boolean()) {
    if (schema.to_boolean()) {
      result.assign("kind", sourcemeta::core::JSON{"any"});
    } else {
      result.assign("kind", sourcemeta::core::JSON{"never"});
    }
    return result;
  }

  if (!schema.is_object()) {
    return result;
  }

  if (schema.defines("$ref") && schema.at("$ref").is_string()) {
    const auto &destination{schema.at("$ref").to_string()};
    const auto target{resolve_destination(destination, frame)};
    if (!target.has_value()) {
      result.assign("kind", sourcemeta::core::JSON{"externalRef"});
      result.assign("url", sourcemeta::core::JSON{destination});
      return result;
    }
    const auto &target_schema{
        sourcemeta::core::get(root, target->get().pointer)};
    const auto visited_entry{visited.find(&target_schema)};
    if (visited_entry != visited.end()) {
      result.assign("kind", sourcemeta::core::JSON{"recursiveRef"});
      result.assign("identifier",
                    sourcemeta::core::JSON{static_cast<std::int64_t>(
                        visited_entry->second.identifier)});
      result.assign("path", visited_entry->second.path);
      return result;
    }

    if (ref_chain.contains(&target_schema)) {
      result.assign("kind", sourcemeta::core::JSON{"any"});
      return result;
    }

    ref_chain.insert(&target_schema);
    auto ref_result{
        type_expression_of(target_schema, frame, root, visited, ref_chain)};
    ref_chain.erase(&target_schema);
    return ref_result;
  }

  if (schema.defines("$dynamicRef") && schema.at("$dynamicRef").is_string()) {
    const auto &value{schema.at("$dynamicRef").to_string()};
    result.assign("kind", sourcemeta::core::JSON{"dynamicRef"});
    const auto fragment_start{value.find('#')};
    if (fragment_start != sourcemeta::core::JSON::String::npos) {
      result.assign("anchor",
                    sourcemeta::core::JSON{value.substr(fragment_start + 1)});
    } else {
      result.assign("anchor", sourcemeta::core::JSON{value});
    }
    return result;
  }

  if (schema.defines("enum") && schema.at("enum").is_array()) {
    result.assign("kind", sourcemeta::core::JSON{"enum"});
    auto values{sourcemeta::core::JSON::make_array()};
    auto overflow{sourcemeta::core::JSON::make_array()};
    std::size_t index{0};
    for (const auto &value : schema.at("enum").as_array()) {
      if (index < 10) {
        values.push_back(value);
      } else {
        overflow.push_back(value);
      }
      ++index;
    }
    result.assign("values", std::move(values));
    if (!overflow.empty()) {
      result.assign("overflow", std::move(overflow));
    }
    return result;
  }

  if (!schema.defines("type") || !schema.at("type").is_string()) {
    result.assign("kind", sourcemeta::core::JSON{"any"});
    return result;
  }

  {
    const auto &type{schema.at("type").to_string()};
    if (type == "object") {
      result.assign("kind", sourcemeta::core::JSON{"object"});
    } else if (type == "array") {
      if (schema.defines("prefixItems") &&
          schema.at("prefixItems").is_array()) {
        result.assign("kind", sourcemeta::core::JSON{"tuple"});
        auto items{sourcemeta::core::JSON::make_array()};
        for (const auto &item : schema.at("prefixItems").as_array()) {
          items.push_back(
              type_expression_of(item, frame, root, visited, ref_chain));
        }
        result.assign("items", std::move(items));
        if (schema.defines("items") && schema.at("items").is_object()) {
          result.assign("additional",
                        type_expression_of(schema.at("items"), frame, root,
                                           visited, ref_chain));
        } else if (schema.defines("unevaluatedItems") &&
                   schema.at("unevaluatedItems").is_object()) {
          result.assign("additional",
                        type_expression_of(schema.at("unevaluatedItems"), frame,
                                           root, visited, ref_chain));
        }
      } else if (schema.defines("items") && schema.at("items").is_array()) {
        result.assign("kind", sourcemeta::core::JSON{"tuple"});
        auto items{sourcemeta::core::JSON::make_array()};
        for (const auto &item : schema.at("items").as_array()) {
          items.push_back(
              type_expression_of(item, frame, root, visited, ref_chain));
        }
        result.assign("items", std::move(items));
        if (schema.defines("additionalItems") &&
            schema.at("additionalItems").is_object()) {
          result.assign("additional",
                        type_expression_of(schema.at("additionalItems"), frame,
                                           root, visited, ref_chain));
        }
      } else {
        result.assign("kind", sourcemeta::core::JSON{"array"});
        if (schema.defines("items") && schema.at("items").is_object()) {
          result.assign("items", type_expression_of(schema.at("items"), frame,
                                                    root, visited, ref_chain));
        }
      }
    } else if (type == "string") {
      result.assign("kind", sourcemeta::core::JSON{"primitive"});
      result.assign("name", sourcemeta::core::JSON{"string"});
    } else if (type == "integer") {
      result.assign("kind", sourcemeta::core::JSON{"primitive"});
      result.assign("name", sourcemeta::core::JSON{"integer"});
    } else if (type == "number") {
      result.assign("kind", sourcemeta::core::JSON{"primitive"});
      result.assign("name", sourcemeta::core::JSON{"number"});
    }
  }

  return result;
}

auto type_expression_of(const sourcemeta::core::JSON &schema,
                        const sourcemeta::core::SchemaFrame &frame,
                        const sourcemeta::core::JSON &root,
                        const VisitedSchemas &visited)
    -> sourcemeta::core::JSON {
  RefChain ref_chain;
  return type_expression_of(schema, frame, root, visited, ref_chain);
}

auto badges_of(const sourcemeta::core::JSON &schema) -> sourcemeta::core::JSON {
  auto badges{sourcemeta::core::JSON::make_array()};
  if (!schema.is_object()) {
    return badges;
  }
  if (schema.defines("format") && schema.at("format").is_string()) {
    auto badge{sourcemeta::core::JSON::make_object()};
    badge.assign("kind", sourcemeta::core::JSON{"format"});
    badge.assign("value",
                 sourcemeta::core::JSON{schema.at("format").to_string()});
    badges.push_back(std::move(badge));
  }
  if (schema.defines("contentEncoding") &&
      schema.at("contentEncoding").is_string()) {
    auto badge{sourcemeta::core::JSON::make_object()};
    badge.assign("kind", sourcemeta::core::JSON{"encoding"});
    badge.assign("value", sourcemeta::core::JSON{
                              schema.at("contentEncoding").to_string()});
    badges.push_back(std::move(badge));
  }
  if (schema.defines("contentMediaType") &&
      schema.at("contentMediaType").is_string()) {
    auto badge{sourcemeta::core::JSON::make_object()};
    badge.assign("kind", sourcemeta::core::JSON{"mime"});
    badge.assign("value", sourcemeta::core::JSON{
                              schema.at("contentMediaType").to_string()});
    badges.push_back(std::move(badge));
  }
  return badges;
}

auto modifiers_of(const sourcemeta::core::JSON &schema)
    -> sourcemeta::core::JSON {
  auto modifiers{sourcemeta::core::JSON::make_array()};
  if (!schema.is_object()) {
    return modifiers;
  }
  if (schema.defines("readOnly") && schema.at("readOnly").is_boolean() &&
      schema.at("readOnly").to_boolean()) {
    modifiers.push_back(sourcemeta::core::JSON{"readOnly"});
  }
  if (schema.defines("writeOnly") && schema.at("writeOnly").is_boolean() &&
      schema.at("writeOnly").to_boolean()) {
    modifiers.push_back(sourcemeta::core::JSON{"writeOnly"});
  }
  if (schema.defines("deprecated") && schema.at("deprecated").is_boolean() &&
      schema.at("deprecated").to_boolean()) {
    modifiers.push_back(sourcemeta::core::JSON{"deprecated"});
  }
  return modifiers;
}

auto format_json_number(const sourcemeta::core::JSON &value)
    -> sourcemeta::core::JSON::String {
  std::ostringstream result;
  sourcemeta::core::stringify(value, result);
  return result.str();
}

auto constraints_of(const sourcemeta::core::JSON &schema)
    -> sourcemeta::core::JSON {
  auto constraints{sourcemeta::core::JSON::make_array()};
  if (!schema.is_object()) {
    return constraints;
  }

  const auto has_min_length{schema.defines("minLength") &&
                            schema.at("minLength").is_integer()};
  const auto has_max_length{schema.defines("maxLength") &&
                            schema.at("maxLength").is_integer()};
  if (has_min_length && has_max_length &&
      schema.at("minLength") == schema.at("maxLength")) {
    const auto value{schema.at("minLength").to_integer()};
    if (value != 0) {
      constraints.push_back(sourcemeta::core::JSON{
          "exactly " + std::to_string(value) + " chars"});
    }
  } else {
    if (has_min_length) {
      const auto value{schema.at("minLength").to_integer()};
      if (value > 0) {
        constraints.push_back(
            sourcemeta::core::JSON{">= " + std::to_string(value) + " chars"});
      }
    }
    if (has_max_length) {
      constraints.push_back(sourcemeta::core::JSON{
          "<= " + std::to_string(schema.at("maxLength").to_integer()) +
          " chars"});
    }
  }

  if (schema.defines("minimum") && schema.at("minimum").is_number()) {
    const auto exclusive{schema.defines("exclusiveMinimum") &&
                         schema.at("exclusiveMinimum").is_boolean() &&
                         schema.at("exclusiveMinimum").to_boolean()};
    constraints.push_back(sourcemeta::core::JSON{
        (exclusive ? "> " : ">= ") + format_json_number(schema.at("minimum"))});
  }
  if (schema.defines("maximum") && schema.at("maximum").is_number()) {
    const auto exclusive{schema.defines("exclusiveMaximum") &&
                         schema.at("exclusiveMaximum").is_boolean() &&
                         schema.at("exclusiveMaximum").to_boolean()};
    constraints.push_back(sourcemeta::core::JSON{
        (exclusive ? "< " : "<= ") + format_json_number(schema.at("maximum"))});
  }
  if (schema.defines("exclusiveMinimum") &&
      schema.at("exclusiveMinimum").is_number()) {
    constraints.push_back(sourcemeta::core::JSON{
        "> " + format_json_number(schema.at("exclusiveMinimum"))});
  }
  if (schema.defines("exclusiveMaximum") &&
      schema.at("exclusiveMaximum").is_number()) {
    constraints.push_back(sourcemeta::core::JSON{
        "< " + format_json_number(schema.at("exclusiveMaximum"))});
  }

  if (schema.defines("multipleOf") && schema.at("multipleOf").is_number()) {
    const auto &value{schema.at("multipleOf")};
    if (!value.is_integer() || value.to_integer() != 1) {
      constraints.push_back(
          sourcemeta::core::JSON{"multiple of " + format_json_number(value)});
    }
  }

  if (schema.defines("minItems") && schema.at("minItems").is_integer()) {
    const auto value{schema.at("minItems").to_integer()};
    if (value > 0) {
      constraints.push_back(
          sourcemeta::core::JSON{">= " + std::to_string(value) + " items"});
    }
  }
  if (schema.defines("maxItems") && schema.at("maxItems").is_integer()) {
    constraints.push_back(sourcemeta::core::JSON{
        "<= " + std::to_string(schema.at("maxItems").to_integer()) + " items"});
  }

  if (schema.defines("uniqueItems") && schema.at("uniqueItems").is_boolean() &&
      schema.at("uniqueItems").to_boolean()) {
    constraints.push_back(sourcemeta::core::JSON{"unique"});
  }

  if (schema.defines("minProperties") &&
      schema.at("minProperties").is_integer()) {
    const auto value{schema.at("minProperties").to_integer()};
    if (value > 0) {
      bool covered_by_required{false};
      if (schema.defines("required") && schema.at("required").is_array() &&
          schema.defines("properties") && schema.at("properties").is_object() &&
          std::cmp_equal(schema.at("required").size(), value)) {
        covered_by_required = true;
        for (const auto &req : schema.at("required").as_array()) {
          if (!req.is_string() ||
              !schema.at("properties").defines(req.to_string())) {
            covered_by_required = false;
            break;
          }
        }
      }
      if (!covered_by_required) {
        constraints.push_back(sourcemeta::core::JSON{
            ">= " + std::to_string(value) + " properties"});
      }
    }
  }
  if (schema.defines("maxProperties") &&
      schema.at("maxProperties").is_integer()) {
    constraints.push_back(sourcemeta::core::JSON{
        "<= " + std::to_string(schema.at("maxProperties").to_integer()) +
        " properties"});
  }

  if (schema.defines("pattern") && schema.at("pattern").is_string()) {
    constraints.push_back(
        sourcemeta::core::JSON{"pattern: " + schema.at("pattern").to_string()});
  }

  const auto has_trivial_contains{schema.defines("contains") &&
                                  schema.at("contains").is_boolean() &&
                                  schema.at("contains").to_boolean()};

  if (schema.defines("contains") && schema.at("contains").is_object()) {
    const auto &contains_schema{schema.at("contains")};
    const auto is_flat{!contains_schema.defines("anyOf") &&
                       !contains_schema.defines("oneOf") &&
                       !contains_schema.defines("allOf") &&
                       !contains_schema.defines("not") &&
                       !contains_schema.defines("enum")};
    if (is_flat) {
      if (contains_schema.defines("type") &&
          contains_schema.at("type").is_string()) {
        constraints.push_back(sourcemeta::core::JSON{
            "contains: " + contains_schema.at("type").to_string()});
      }

      const auto inner{constraints_of(contains_schema)};
      for (const auto &constraint : inner.as_array()) {
        constraints.push_back(
            sourcemeta::core::JSON{"contains " + constraint.to_string()});
      }
    }
  }

  const auto has_min_contains{!has_trivial_contains &&
                              schema.defines("minContains") &&
                              schema.at("minContains").is_integer()};
  const auto has_max_contains{!has_trivial_contains &&
                              schema.defines("maxContains") &&
                              schema.at("maxContains").is_integer()};
  if (has_min_contains && has_max_contains &&
      schema.at("minContains") == schema.at("maxContains")) {
    constraints.push_back(sourcemeta::core::JSON{
        "exactly " + std::to_string(schema.at("minContains").to_integer()) +
        " matching items"});
  } else {
    if (has_min_contains) {
      const auto value{schema.at("minContains").to_integer()};
      if (value == 0) {
        constraints.push_back(
            sourcemeta::core::JSON{"0 or more matching items"});
      } else {
        constraints.push_back(sourcemeta::core::JSON{
            ">= " + std::to_string(value) + " matching items"});
      }
    }
    if (has_max_contains) {
      constraints.push_back(sourcemeta::core::JSON{
          "<= " + std::to_string(schema.at("maxContains").to_integer()) +
          " matching items"});
    }
  }

  if (schema.defines("propertyNames") &&
      schema.at("propertyNames").is_object()) {
    const auto &names_schema{schema.at("propertyNames")};
    const auto is_branching{
        names_schema.defines("anyOf") || names_schema.defines("oneOf") ||
        names_schema.defines("allOf") || names_schema.defines("not")};
    if (!is_branching) {
      const auto inner{constraints_of(names_schema)};
      if (inner.empty() && names_schema.defines("type") &&
          names_schema.at("type").is_string()) {
        constraints.push_back(sourcemeta::core::JSON{
            "keys: " + names_schema.at("type").to_string()});
      }

      for (const auto &constraint : inner.as_array()) {
        constraints.push_back(
            sourcemeta::core::JSON{"keys " + constraint.to_string()});
      }
    }
  }

  if (schema.defines("contentSchema") &&
      schema.at("contentSchema").is_object()) {
    const auto &content_schema{schema.at("contentSchema")};
    const auto is_branching{
        content_schema.defines("anyOf") || content_schema.defines("oneOf") ||
        content_schema.defines("allOf") || content_schema.defines("not")};
    if (!is_branching) {
      const auto inner{constraints_of(content_schema)};
      if (inner.empty() && content_schema.defines("type") &&
          content_schema.at("type").is_string()) {
        constraints.push_back(sourcemeta::core::JSON{
            "decoded: " + content_schema.at("type").to_string()});
      }

      for (const auto &constraint : inner.as_array()) {
        constraints.push_back(
            sourcemeta::core::JSON{"decoded " + constraint.to_string()});
      }
    }
  }

  if (schema.defines("not") && schema.at("not").is_object()) {
    const auto &not_schema{schema.at("not")};
    const auto is_branching{
        not_schema.defines("anyOf") || not_schema.defines("oneOf") ||
        not_schema.defines("allOf") || not_schema.defines("not")};
    if (!is_branching) {
      const auto inner{constraints_of(not_schema)};
      for (const auto &constraint : inner.as_array()) {
        constraints.push_back(
            sourcemeta::core::JSON{"must NOT match " + constraint.to_string()});
      }
    }
  }

  return constraints;
}

auto is_required_property(const sourcemeta::core::JSON &schema,
                          const sourcemeta::core::JSON::String &property)
    -> bool {
  if (!schema.is_object() || !schema.defines("required") ||
      !schema.at("required").is_array()) {
    return false;
  }
  for (const auto &item : schema.at("required").as_array()) {
    if (item.is_string() && item.to_string() == property) {
      return true;
    }
  }
  return false;
}

auto make_path_segment(const sourcemeta::core::JSON::String &type,
                       const sourcemeta::core::JSON::String &value)
    -> sourcemeta::core::JSON {
  auto segment{sourcemeta::core::JSON::make_object()};
  segment.assign("type", sourcemeta::core::JSON{type});
  segment.assign("value", sourcemeta::core::JSON{value});
  return segment;
}

auto make_section(const std::string &label, sourcemeta::core::JSON tables)
    -> sourcemeta::core::JSON {
  auto section{sourcemeta::core::JSON::make_object()};
  section.assign("label", sourcemeta::core::JSON{label});
  section.assign("children", std::move(tables));
  return section;
}

auto walk_schema(const sourcemeta::core::JSON &schema, bool include_root,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                 std::size_t &next_identifier) -> sourcemeta::core::JSON;

auto is_complex_schema(const sourcemeta::core::JSON &schema) -> bool;

auto walk_branching_subschema(const std::string &label,
                              const std::string &synthetic_name,
                              const sourcemeta::core::JSON &inner_schema,
                              sourcemeta::core::JSON &doc_children,
                              const sourcemeta::core::SchemaFrame &frame,
                              const sourcemeta::core::JSON &root,
                              VisitedSchemas &visited,
                              std::size_t &next_identifier,
                              bool include_properties) -> void;

auto walk_branches(const std::string &keyword, const std::string &label,
                   const sourcemeta::core::JSON &schema,
                   sourcemeta::core::JSON &children,
                   const sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                   std::size_t &next_identifier) -> void;

auto walk_all_of(const sourcemeta::core::JSON &schema,
                 sourcemeta::core::JSON &rows, sourcemeta::core::JSON &children,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                 std::size_t &next_identifier) -> void;

auto walk_if_then_else(const sourcemeta::core::JSON &schema,
                       sourcemeta::core::JSON &children,
                       const sourcemeta::core::SchemaFrame &frame,
                       const sourcemeta::core::JSON &root,
                       VisitedSchemas &visited, std::size_t &next_identifier)
    -> void;

auto walk_wildcard_keyword(const sourcemeta::core::JSON &schema,
                           const std::string &keyword,
                           const sourcemeta::core::JSON &base_path,
                           sourcemeta::core::JSON &rows,
                           const sourcemeta::core::SchemaFrame &frame,
                           const sourcemeta::core::JSON &root,
                           VisitedSchemas &visited,
                           std::size_t &next_identifier) -> void;

auto walk_pattern_properties(const sourcemeta::core::JSON &schema,
                             const sourcemeta::core::JSON &base_path,
                             sourcemeta::core::JSON &rows,
                             const sourcemeta::core::SchemaFrame &frame,
                             const sourcemeta::core::JSON &root,
                             VisitedSchemas &visited,
                             std::size_t &next_identifier) -> void;

auto resolve_ref(const sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::JSON &root,
                 const VisitedSchemas &visited)
    -> const sourcemeta::core::JSON & {
  if (schema.is_object() && schema.defines("$ref") &&
      schema.at("$ref").is_string()) {
    const auto target{
        resolve_destination(schema.at("$ref").to_string(), frame)};
    if (target.has_value()) {
      const auto &target_schema{
          sourcemeta::core::get(root, target->get().pointer)};
      if (visited.find(&target_schema) != visited.end()) {
        return schema; // NOLINT(bugprone-return-const-ref-from-parameter)
      }
      return target_schema;
    }
  }
  return schema; // NOLINT(bugprone-return-const-ref-from-parameter)
}

auto emit_row(const sourcemeta::core::JSON &schema, sourcemeta::core::JSON path,
              sourcemeta::core::JSON &rows,
              const sourcemeta::core::SchemaFrame &frame,
              const sourcemeta::core::JSON &root, VisitedSchemas &visited,
              std::size_t &next_identifier,
              const bool expand_applicators = true) -> void {
  auto row{sourcemeta::core::JSON::make_object()};
  row.assign("identifier", sourcemeta::core::JSON{
                               static_cast<std::int64_t>(next_identifier++)});
  row.assign("path", std::move(path));

  auto modifiers{modifiers_of(schema)};
  if (!modifiers.empty()) {
    row.assign("modifiers", std::move(modifiers));
  }

  row.assign("type", type_expression_of(schema, frame, root, visited));

  auto badges{badges_of(schema)};
  if (!badges.empty()) {
    row.assign("badges", std::move(badges));
  }

  auto constraints{constraints_of(schema)};
  if (!constraints.empty()) {
    row.assign("constraints", std::move(constraints));
  }

  if (schema.is_object()) {
    if (schema.defines("title") && schema.at("title").is_string()) {
      row.assign("title",
                 sourcemeta::core::JSON{schema.at("title").to_string()});
    }
    if (schema.defines("description") && schema.at("description").is_string()) {
      row.assign("description",
                 sourcemeta::core::JSON{schema.at("description").to_string()});
    }
    if (schema.defines("default")) {
      row.assign("default", schema.at("default"));
    }
    if (schema.defines("examples") && schema.at("examples").is_array()) {
      auto examples{sourcemeta::core::JSON::make_array()};
      for (const auto &example : schema.at("examples").as_array()) {
        examples.push_back(example);
      }
      row.assign("examples", std::move(examples));
    }
  }

  if (expand_applicators && is_complex_schema(schema)) {
    auto row_children{sourcemeta::core::JSON::make_array()};
    walk_branches("anyOf", "Any of", schema, row_children, frame, root, visited,
                  next_identifier);
    walk_branches("oneOf", "One of", schema, row_children, frame, root, visited,
                  next_identifier);
    walk_all_of(schema, rows, row_children, frame, root, visited,
                next_identifier);
    walk_if_then_else(schema, row_children, frame, root, visited,
                      next_identifier);
    if (schema.is_object() && schema.defines("not")) {
      const auto &not_schema{schema.at("not")};
      const auto has_inline{
          not_schema.is_object() &&
          !(not_schema.defines("anyOf") || not_schema.defines("oneOf") ||
            not_schema.defines("allOf") || not_schema.defines("not")) &&
          !constraints_of(not_schema).empty()};
      if (!has_inline) {
        walk_branching_subschema("Must NOT match", "value", not_schema,
                                 row_children, frame, root, visited,
                                 next_identifier, false);
      }
    }
    if (!row_children.empty()) {
      row.assign("children", std::move(row_children));
    }
  }

  rows.push_back(std::move(row));
}

auto walk_properties(const sourcemeta::core::JSON &schema,
                     const sourcemeta::core::JSON &base_path,
                     sourcemeta::core::JSON &rows,
                     const sourcemeta::core::SchemaFrame &frame,
                     const sourcemeta::core::JSON &root,
                     VisitedSchemas &visited, std::size_t &next_identifier)
    -> void {
  if (!schema.is_object() || !schema.defines("properties") ||
      !schema.at("properties").is_object()) {
    return;
  }

  for (const auto &entry : schema.at("properties").as_object()) {
    const auto &resolved{resolve_ref(entry.second, frame, root, visited)};
    auto path{base_path};
    path.push_back(make_path_segment("literal", entry.first));

    auto row{sourcemeta::core::JSON::make_object()};
    row.assign("identifier", sourcemeta::core::JSON{
                                 static_cast<std::int64_t>(next_identifier++)});
    row.assign("path", path);

    auto modifiers{modifiers_of(resolved)};
    if (!modifiers.empty()) {
      row.assign("modifiers", std::move(modifiers));
    }

    row.assign("type", type_expression_of(resolved, frame, root, visited));

    auto badges{badges_of(resolved)};
    if (!badges.empty()) {
      row.assign("badges", std::move(badges));
    }

    row.assign("required", sourcemeta::core::JSON{
                               is_required_property(schema, entry.first)});

    auto constraints{constraints_of(resolved)};
    if (!constraints.empty()) {
      row.assign("constraints", std::move(constraints));
    }

    if (resolved.is_object()) {
      if (resolved.defines("title") && resolved.at("title").is_string()) {
        row.assign("title",
                   sourcemeta::core::JSON{resolved.at("title").to_string()});
      }
      if (resolved.defines("description") &&
          resolved.at("description").is_string()) {
        row.assign("description", sourcemeta::core::JSON{
                                      resolved.at("description").to_string()});
      }
      if (resolved.defines("default")) {
        row.assign("default", resolved.at("default"));
      }
      if (resolved.defines("examples") && resolved.at("examples").is_array()) {
        auto examples{sourcemeta::core::JSON::make_array()};
        for (const auto &example : resolved.at("examples").as_array()) {
          examples.push_back(example);
        }
        row.assign("examples", std::move(examples));
      }
    }

    const auto row_identifier{
        static_cast<std::size_t>(row.at("identifier").to_integer())};

    if (is_complex_schema(resolved)) {
      auto prop_children{sourcemeta::core::JSON::make_array()};
      walk_branches("anyOf", "Any of", resolved, prop_children, frame, root,
                    visited, next_identifier);
      walk_branches("oneOf", "One of", resolved, prop_children, frame, root,
                    visited, next_identifier);
      walk_all_of(resolved, rows, prop_children, frame, root, visited,
                  next_identifier);
      walk_if_then_else(resolved, prop_children, frame, root, visited,
                        next_identifier);
      if (resolved.defines("not")) {
        const auto &not_schema{resolved.at("not")};
        const auto has_inline{
            not_schema.is_object() &&
            !(not_schema.defines("anyOf") || not_schema.defines("oneOf") ||
              not_schema.defines("allOf") || not_schema.defines("not")) &&
            !constraints_of(not_schema).empty()};
        if (!has_inline) {
          walk_branching_subschema("Must NOT match", "value", not_schema,
                                   prop_children, frame, root, visited,
                                   next_identifier, false);
        }
      }
      if (!prop_children.empty()) {
        row.assign("children", std::move(prop_children));
      }
    }

    rows.push_back(std::move(row));

    if (resolved.is_object() && resolved.defines("type") &&
        resolved.at("type").is_string()) {
      const auto &resolved_type{resolved.at("type").to_string()};
      if (resolved_type == "object") {
        visited.emplace(&resolved, VisitedEntry{.identifier = row_identifier,
                                                .path = path});
        walk_properties(resolved, path, rows, frame, root, visited,
                        next_identifier);
        walk_pattern_properties(resolved, path, rows, frame, root, visited,
                                next_identifier);
        walk_wildcard_keyword(resolved, "additionalProperties", path, rows,
                              frame, root, visited, next_identifier);
        walk_wildcard_keyword(resolved, "unevaluatedProperties", path, rows,
                              frame, root, visited, next_identifier);
        if (!resolved.defines("additionalProperties") &&
            !resolved.defines("unevaluatedProperties")) {
          auto open_path{path};
          open_path.push_back(make_path_segment("wildcard", "*"));
          emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows,
                   frame, root, visited, next_identifier);
        }
        visited.erase(&resolved);
      } else if (resolved_type == "array" && resolved.defines("items") &&
                 resolved.at("items").is_object() &&
                 !resolved.defines("prefixItems")) {
        const auto &items_schema{
            resolve_ref(resolved.at("items"), frame, root, visited)};
        if (items_schema.is_object()) {
          auto wildcard_path{path};
          wildcard_path.push_back(make_path_segment("wildcard", "*"));
          const auto items_row_id{next_identifier};
          emit_row(items_schema, wildcard_path, rows, frame, root, visited,
                   next_identifier);
          if (items_schema.defines("type") &&
              items_schema.at("type").is_string() &&
              items_schema.at("type").to_string() == "object") {
            visited.emplace(&items_schema,
                            VisitedEntry{.identifier = items_row_id,
                                         .path = wildcard_path});
            walk_properties(items_schema, wildcard_path, rows, frame, root,
                            visited, next_identifier);
            walk_pattern_properties(items_schema, wildcard_path, rows, frame,
                                    root, visited, next_identifier);
            walk_wildcard_keyword(items_schema, "additionalProperties",
                                  wildcard_path, rows, frame, root, visited,
                                  next_identifier);
            walk_wildcard_keyword(items_schema, "unevaluatedProperties",
                                  wildcard_path, rows, frame, root, visited,
                                  next_identifier);
            if (!items_schema.defines("additionalProperties") &&
                !items_schema.defines("unevaluatedProperties")) {
              auto open_path{wildcard_path};
              open_path.push_back(make_path_segment("wildcard", "*"));
              emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows,
                       frame, root, visited, next_identifier);
            }
            visited.erase(&items_schema);
          }
        }
      }
    }
  }
}

auto walk_wildcard_keyword(const sourcemeta::core::JSON &schema,
                           const std::string &keyword,
                           const sourcemeta::core::JSON &base_path,
                           sourcemeta::core::JSON &rows,
                           const sourcemeta::core::SchemaFrame &frame,
                           const sourcemeta::core::JSON &root,
                           VisitedSchemas &visited,
                           std::size_t &next_identifier) -> void {
  if (!schema.is_object() || !schema.defines(keyword)) {
    return;
  }

  const auto &value{schema.at(keyword)};

  if (keyword == "unevaluatedItems" && schema.defines("prefixItems")) {
    return;
  }

  if (value.is_boolean() && value.to_boolean()) {
    auto path{base_path};
    path.push_back(make_path_segment("wildcard", "*"));
    emit_row(value, std::move(path), rows, frame, root, visited,
             next_identifier);
    return;
  }

  if (!value.is_object()) {
    return;
  }

  auto path{base_path};
  path.push_back(make_path_segment("wildcard", "*"));
  const auto wildcard_row_id{next_identifier};
  emit_row(value, path, rows, frame, root, visited, next_identifier);

  if (value.defines("type") && value.at("type").is_string() &&
      value.at("type").to_string() == "object") {
    visited.emplace(&value,
                    VisitedEntry{.identifier = wildcard_row_id, .path = path});
    walk_properties(value, path, rows, frame, root, visited, next_identifier);
    walk_pattern_properties(value, path, rows, frame, root, visited,
                            next_identifier);
    walk_wildcard_keyword(value, "additionalProperties", path, rows, frame,
                          root, visited, next_identifier);
    walk_wildcard_keyword(value, "unevaluatedProperties", path, rows, frame,
                          root, visited, next_identifier);
    if (!value.defines("additionalProperties") &&
        !value.defines("unevaluatedProperties")) {
      auto open_path{path};
      open_path.push_back(make_path_segment("wildcard", "*"));
      emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows, frame,
               root, visited, next_identifier);
    }
    visited.erase(&value);
  } else if (value.defines("type") && value.at("type").is_string() &&
             value.at("type").to_string() == "array" &&
             value.defines("items") && value.at("items").is_object() &&
             !value.defines("prefixItems")) {
    const auto &items_schema{
        resolve_ref(value.at("items"), frame, root, visited)};
    if (items_schema.is_object()) {
      auto items_path{path};
      items_path.push_back(make_path_segment("wildcard", "*"));
      const auto items_row_id{next_identifier};
      emit_row(items_schema, items_path, rows, frame, root, visited,
               next_identifier);
      if (items_schema.defines("type") && items_schema.at("type").is_string() &&
          items_schema.at("type").to_string() == "object") {
        visited.emplace(&items_schema, VisitedEntry{.identifier = items_row_id,
                                                    .path = items_path});
        walk_properties(items_schema, items_path, rows, frame, root, visited,
                        next_identifier);
        walk_pattern_properties(items_schema, items_path, rows, frame, root,
                                visited, next_identifier);
        walk_wildcard_keyword(items_schema, "additionalProperties", items_path,
                              rows, frame, root, visited, next_identifier);
        walk_wildcard_keyword(items_schema, "unevaluatedProperties", items_path,
                              rows, frame, root, visited, next_identifier);
        if (!items_schema.defines("additionalProperties") &&
            !items_schema.defines("unevaluatedProperties")) {
          auto open_path{items_path};
          open_path.push_back(make_path_segment("wildcard", "*"));
          emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows,
                   frame, root, visited, next_identifier);
        }
        visited.erase(&items_schema);
      }
    }
  }
}

auto walk_pattern_properties(const sourcemeta::core::JSON &schema,
                             const sourcemeta::core::JSON &base_path,
                             sourcemeta::core::JSON &rows,
                             const sourcemeta::core::SchemaFrame &frame,
                             const sourcemeta::core::JSON &root,
                             VisitedSchemas &visited,
                             std::size_t &next_identifier) -> void {
  if (!schema.is_object() || !schema.defines("patternProperties") ||
      !schema.at("patternProperties").is_object()) {
    return;
  }

  for (const auto &entry : schema.at("patternProperties").as_object()) {
    const auto &resolved{resolve_ref(entry.second, frame, root, visited)};
    auto path{base_path};
    path.push_back(make_path_segment("pattern", entry.first));

    const auto row_id{next_identifier};
    emit_row(resolved, path, rows, frame, root, visited, next_identifier);

    if (resolved.is_object() && resolved.defines("type") &&
        resolved.at("type").is_string() &&
        resolved.at("type").to_string() == "object") {
      visited.emplace(&resolved,
                      VisitedEntry{.identifier = row_id, .path = path});
      walk_properties(resolved, path, rows, frame, root, visited,
                      next_identifier);
      walk_pattern_properties(resolved, path, rows, frame, root, visited,
                              next_identifier);
      walk_wildcard_keyword(resolved, "additionalProperties", path, rows, frame,
                            root, visited, next_identifier);
      walk_wildcard_keyword(resolved, "unevaluatedProperties", path, rows,
                            frame, root, visited, next_identifier);
      if (!resolved.defines("additionalProperties") &&
          !resolved.defines("unevaluatedProperties")) {
        auto open_path{path};
        open_path.push_back(make_path_segment("wildcard", "*"));
        emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows,
                 frame, root, visited, next_identifier);
      }
      visited.erase(&resolved);
    }
  }
}

auto is_complex_schema(const sourcemeta::core::JSON &schema) -> bool {
  if (!schema.is_object()) {
    return false;
  }
  return schema.defines("properties") || schema.defines("anyOf") ||
         schema.defines("oneOf") || schema.defines("allOf") ||
         schema.defines("not") || schema.defines("if") ||
         schema.defines("prefixItems") || schema.defines("contains") ||
         schema.defines("patternProperties") ||
         schema.defines("additionalProperties") ||
         schema.defines("propertyNames") || schema.defines("contentSchema");
}

auto walk_prefix_items(const sourcemeta::core::JSON &schema,
                       const sourcemeta::core::JSON &base_path,
                       sourcemeta::core::JSON &rows,
                       sourcemeta::core::JSON &children,
                       const sourcemeta::core::SchemaFrame &frame,
                       const sourcemeta::core::JSON &root,
                       VisitedSchemas &visited, std::size_t &next_identifier)
    -> void {
  const auto has_prefix_items{schema.is_object() &&
                              schema.defines("prefixItems") &&
                              schema.at("prefixItems").is_array()};
  const auto has_draft4_tuple{!has_prefix_items && schema.is_object() &&
                              schema.defines("items") &&
                              schema.at("items").is_array()};
  if (!has_prefix_items && !has_draft4_tuple) {
    return;
  }

  const auto &tuple_items{has_prefix_items ? schema.at("prefixItems")
                                           : schema.at("items")};

  std::size_t min_items{0};
  if (schema.defines("minItems") && schema.at("minItems").is_integer() &&
      schema.at("minItems").to_integer() > 0) {
    min_items = static_cast<std::size_t>(schema.at("minItems").to_integer());
  }

  std::size_t index{0};
  for (const auto &item : tuple_items.as_array()) {
    if (is_complex_schema(item)) {
      auto section_children{sourcemeta::core::JSON::make_array()};
      section_children.push_back(
          walk_schema(item, true, frame, root, visited, next_identifier));
      children.push_back(make_section("Array item " + std::to_string(index),
                                      std::move(section_children)));
    } else {
      auto path{base_path};
      path.push_back(make_path_segment("literal", std::to_string(index)));

      auto row{sourcemeta::core::JSON::make_object()};
      row.assign("identifier", sourcemeta::core::JSON{static_cast<std::int64_t>(
                                   next_identifier++)});
      row.assign("path", std::move(path));

      auto modifiers{modifiers_of(item)};
      if (!modifiers.empty()) {
        row.assign("modifiers", std::move(modifiers));
      }

      row.assign("type", type_expression_of(item, frame, root, visited));

      auto badges{badges_of(item)};
      if (!badges.empty()) {
        row.assign("badges", std::move(badges));
      }

      row.assign("required", sourcemeta::core::JSON{index < min_items});

      auto constraints{constraints_of(item)};
      if (!constraints.empty()) {
        row.assign("constraints", std::move(constraints));
      }

      if (item.is_object()) {
        if (item.defines("title") && item.at("title").is_string()) {
          row.assign("title",
                     sourcemeta::core::JSON{item.at("title").to_string()});
        }
        if (item.defines("description") && item.at("description").is_string()) {
          row.assign("description", sourcemeta::core::JSON{
                                        item.at("description").to_string()});
        }
        if (item.defines("default")) {
          row.assign("default", item.at("default"));
        }
        if (item.defines("examples") && item.at("examples").is_array()) {
          auto examples{sourcemeta::core::JSON::make_array()};
          for (const auto &example : item.at("examples").as_array()) {
            examples.push_back(example);
          }
          row.assign("examples", std::move(examples));
        }
      }

      rows.push_back(std::move(row));
    }
    ++index;
  }

  if (has_prefix_items && schema.defines("items") &&
      schema.at("items").is_object()) {
    auto path{base_path};
    path.push_back(make_path_segment("wildcard", "*"));
    emit_row(schema.at("items"), std::move(path), rows, frame, root, visited,
             next_identifier);
  } else if (has_draft4_tuple && schema.defines("additionalItems") &&
             schema.at("additionalItems").is_object()) {
    auto path{base_path};
    path.push_back(make_path_segment("wildcard", "*"));
    emit_row(schema.at("additionalItems"), std::move(path), rows, frame, root,
             visited, next_identifier);
  }
}

auto walk_branches(const std::string &keyword, const std::string &label,
                   const sourcemeta::core::JSON &schema,
                   sourcemeta::core::JSON &children,
                   const sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                   std::size_t &next_identifier) -> void {
  if (!schema.is_object() || !schema.defines(keyword) ||
      !schema.at(keyword).is_array()) {
    return;
  }

  auto section_children{sourcemeta::core::JSON::make_array()};
  for (const auto &branch : schema.at(keyword).as_array()) {
    section_children.push_back(
        walk_schema(branch, false, frame, root, visited, next_identifier));
  }
  children.push_back(make_section(label, std::move(section_children)));
}

auto has_recursive_ref_in_rows(const sourcemeta::core::JSON &rows) -> bool {
  for (const auto &row : rows.as_array()) {
    const auto &type{row.at("type")};
    if (type.defines("kind") && type.at("kind").to_string() == "recursiveRef") {
      return true;
    }
    if (type.defines("kind") && type.at("kind").to_string() == "array" &&
        type.defines("items")) {
      const auto &items{type.at("items")};
      if (items.is_object() && items.defines("kind") &&
          items.at("kind").to_string() == "recursiveRef") {
        return true;
      }
    }
  }
  return false;
}

auto walk_all_of(const sourcemeta::core::JSON &schema,
                 sourcemeta::core::JSON &rows, sourcemeta::core::JSON &children,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                 std::size_t &next_identifier) -> void {
  if (!schema.is_object() || !schema.defines("allOf") ||
      !schema.at("allOf").is_array()) {
    return;
  }

  const auto &all_of{schema.at("allOf").as_array()};

  if (all_of.size() == 1) {
    auto branch{walk_schema(schema.at("allOf").at(0), false, frame, root,
                            visited, next_identifier)};
    const auto &branch_rows{branch.at("rows")};
    const auto branch_has_recursive_ref{has_recursive_ref_in_rows(branch_rows)};

    if (!branch_has_recursive_ref) {
      // Merge the branch's root row info into the parent's root row
      if (!rows.empty() && !branch_rows.empty()) {
        const auto &parent_last_type{rows.at(rows.size() - 1).at("type")};
        const auto &branch_first{branch_rows.at(0)};
        const auto parent_is_any{parent_last_type.defines("kind") &&
                                 parent_last_type.at("kind").to_string() ==
                                     "any"};
        const auto &branch_first_path{branch_first.at("path")};
        const auto branch_first_is_synthetic_root{
            !branch_first_path.empty() &&
            branch_first_path.at(0).at("type").to_string() == "synthetic" &&
            branch_first_path.at(0).at("value").to_string() == "root"};

        if (parent_is_any && branch_first_is_synthetic_root) {
          // Copy fields from branch root into parent root
          auto &parent_root{rows.at(rows.size() - 1)};
          parent_root.assign("type", branch_first.at("type"));

          if (branch_first.defines("constraints")) {
            parent_root.assign("constraints", branch_first.at("constraints"));
          } else if (parent_root.defines("constraints")) {
            parent_root.erase("constraints");
          }

          if (branch_first.defines("title") && !parent_root.defines("title")) {
            parent_root.assign("title", branch_first.at("title"));
          }
          if (branch_first.defines("description") &&
              !parent_root.defines("description")) {
            parent_root.assign("description", branch_first.at("description"));
          }
          if (branch_first.defines("default") &&
              !parent_root.defines("default")) {
            parent_root.assign("default", branch_first.at("default"));
          }

          if (branch_first.defines("modifiers")) {
            parent_root.assign("modifiers", branch_first.at("modifiers"));
          } else if (parent_root.defines("modifiers")) {
            parent_root.erase("modifiers");
          }

          if (branch_first.defines("badges")) {
            parent_root.assign("badges", branch_first.at("badges"));
          } else if (parent_root.defines("badges")) {
            parent_root.erase("badges");
          }

          for (std::size_t index = 1; index < branch_rows.size(); ++index) {
            rows.push_back(branch_rows.at(index));
          }
        } else {
          for (const auto &row : branch_rows.as_array()) {
            rows.push_back(row);
          }
        }
      } else {
        for (const auto &row : branch_rows.as_array()) {
          rows.push_back(row);
        }
      }

      if (branch.defines("children")) {
        for (const auto &child : branch.at("children").as_array()) {
          children.push_back(child);
        }
      }
      return;
    }

    auto section_children{sourcemeta::core::JSON::make_array()};
    section_children.push_back(std::move(branch));
    children.push_back(make_section("All of", std::move(section_children)));
    return;
  }

  auto section_children{sourcemeta::core::JSON::make_array()};
  for (const auto &branch : all_of) {
    section_children.push_back(
        walk_schema(branch, false, frame, root, visited, next_identifier));
  }
  children.push_back(make_section("All of", std::move(section_children)));
}

auto walk_if_then_else(const sourcemeta::core::JSON &schema,
                       sourcemeta::core::JSON &children,
                       const sourcemeta::core::SchemaFrame &frame,
                       const sourcemeta::core::JSON &root,
                       VisitedSchemas &visited, std::size_t &next_identifier)
    -> void {
  if (!schema.is_object() || !schema.defines("if") || !schema.defines("then") ||
      !schema.defines("else")) {
    return;
  }

  {
    auto section_children{sourcemeta::core::JSON::make_array()};
    section_children.push_back(walk_schema(schema.at("if"), false, frame, root,
                                           visited, next_identifier));
    children.push_back(make_section("If", std::move(section_children)));
  }

  {
    auto section_children{sourcemeta::core::JSON::make_array()};
    section_children.push_back(walk_schema(schema.at("then"), false, frame,
                                           root, visited, next_identifier));
    children.push_back(make_section("Then", std::move(section_children)));
  }

  {
    auto section_children{sourcemeta::core::JSON::make_array()};
    section_children.push_back(walk_schema(schema.at("else"), false, frame,
                                           root, visited, next_identifier));
    children.push_back(make_section("Else", std::move(section_children)));
  }
}

auto walk_branching_subschema(const std::string &label,
                              const std::string &synthetic_name,
                              const sourcemeta::core::JSON &inner_schema,
                              sourcemeta::core::JSON &doc_children,
                              const sourcemeta::core::SchemaFrame &frame,
                              const sourcemeta::core::JSON &root,
                              VisitedSchemas &visited,
                              std::size_t &next_identifier,
                              const bool include_properties) -> void {
  auto table{sourcemeta::core::JSON::make_object()};
  table.assign("identifier", sourcemeta::core::JSON{
                                 static_cast<std::int64_t>(next_identifier++)});
  auto table_rows{sourcemeta::core::JSON::make_array()};
  auto table_children{sourcemeta::core::JSON::make_array()};
  auto synthetic_path{sourcemeta::core::JSON::make_array()};
  synthetic_path.push_back(make_path_segment("synthetic", synthetic_name));
  if (include_properties) {
    walk_properties(inner_schema, synthetic_path, table_rows, frame, root,
                    visited, next_identifier);
  }
  emit_row(inner_schema, std::move(synthetic_path), table_rows, frame, root,
           visited, next_identifier, false);
  walk_branches("anyOf", "Any of", inner_schema, table_children, frame, root,
                visited, next_identifier);
  walk_branches("oneOf", "One of", inner_schema, table_children, frame, root,
                visited, next_identifier);
  walk_all_of(inner_schema, table_rows, table_children, frame, root, visited,
              next_identifier);
  table.assign("rows", std::move(table_rows));
  if (!table_children.empty()) {
    table.assign("children", std::move(table_children));
  }
  auto section_children{sourcemeta::core::JSON::make_array()};
  section_children.push_back(std::move(table));
  doc_children.push_back(make_section(label, std::move(section_children)));
}

auto walk_schema(const sourcemeta::core::JSON &schema, const bool include_root,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::JSON &root, VisitedSchemas &visited,
                 std::size_t &next_identifier) -> sourcemeta::core::JSON {
  if (schema.is_object() && schema.defines("$ref") &&
      schema.at("$ref").is_string()) {
    const auto target{
        resolve_destination(schema.at("$ref").to_string(), frame)};
    if (target.has_value()) {
      const auto &target_schema{
          sourcemeta::core::get(root, target->get().pointer)};
      const auto visited_entry{visited.find(&target_schema)};
      if (visited_entry != visited.end()) {
        auto documentation{sourcemeta::core::JSON::make_object()};
        documentation.assign("identifier",
                             sourcemeta::core::JSON{
                                 static_cast<std::int64_t>(next_identifier++)});
        auto rows{sourcemeta::core::JSON::make_array()};
        auto row{sourcemeta::core::JSON::make_object()};
        row.assign("identifier",
                   sourcemeta::core::JSON{
                       static_cast<std::int64_t>(next_identifier++)});
        auto path{sourcemeta::core::JSON::make_array()};
        path.push_back(make_path_segment("synthetic", "root"));
        row.assign("path", std::move(path));
        auto type_expr{sourcemeta::core::JSON::make_object()};
        type_expr.assign("kind", sourcemeta::core::JSON{"recursiveRef"});
        type_expr.assign("identifier",
                         sourcemeta::core::JSON{static_cast<std::int64_t>(
                             visited_entry->second.identifier)});
        type_expr.assign("path", visited_entry->second.path);
        row.assign("type", std::move(type_expr));
        rows.push_back(std::move(row));
        documentation.assign("rows", std::move(rows));
        return documentation;
      }
      auto ref_path{sourcemeta::core::JSON::make_array()};
      ref_path.push_back(make_path_segment("synthetic", "root"));
      visited.emplace(&target_schema,
                      VisitedEntry{.identifier = next_identifier,
                                   .path = std::move(ref_path)});
      auto result{walk_schema(target_schema, include_root, frame, root, visited,
                              next_identifier)};
      visited.erase(&target_schema);
      return result;
    }
  }

  auto documentation{sourcemeta::core::JSON::make_object()};
  const auto doc_identifier{next_identifier++};
  documentation.assign(
      "identifier",
      sourcemeta::core::JSON{static_cast<std::int64_t>(doc_identifier)});

  if (schema.is_object() && schema.defines("$dynamicAnchor") &&
      schema.at("$dynamicAnchor").is_string()) {
    documentation.assign(
        "dynamicAnchor",
        sourcemeta::core::JSON{schema.at("$dynamicAnchor").to_string()});
  }

  auto rows{sourcemeta::core::JSON::make_array()};
  auto doc_children{sourcemeta::core::JSON::make_array()};

  if (include_root) {
    auto root_path{sourcemeta::core::JSON::make_array()};
    root_path.push_back(make_path_segment("synthetic", "root"));
    emit_row(schema, std::move(root_path), rows, frame, root, visited,
             next_identifier, false);
    const auto root_row_identifier{static_cast<std::size_t>(
        rows.at(rows.size() - 1).at("identifier").to_integer())};
    auto visited_root_path{sourcemeta::core::JSON::make_array()};
    visited_root_path.push_back(make_path_segment("synthetic", "root"));
    visited.emplace(&schema,
                    VisitedEntry{.identifier = root_row_identifier,
                                 .path = std::move(visited_root_path)});
  }

  if (!schema.is_object()) {
    if (!include_root) {
      auto root_path{sourcemeta::core::JSON::make_array()};
      root_path.push_back(make_path_segment("synthetic", "root"));
      emit_row(schema, std::move(root_path), rows, frame, root, visited,
               next_identifier, false);
    }
    documentation.assign("rows", std::move(rows));
    return documentation;
  }

  if (!include_root) {
    auto root_path{sourcemeta::core::JSON::make_array()};
    root_path.push_back(make_path_segment("synthetic", "root"));
    emit_row(schema, std::move(root_path), rows, frame, root, visited,
             next_identifier, false);
  }

  const auto empty_path{sourcemeta::core::JSON::make_array()};
  walk_properties(schema, empty_path, rows, frame, root, visited,
                  next_identifier);
  walk_pattern_properties(schema, empty_path, rows, frame, root, visited,
                          next_identifier);
  walk_wildcard_keyword(schema, "additionalProperties", empty_path, rows, frame,
                        root, visited, next_identifier);
  if (schema.defines("type") && schema.at("type").is_string() &&
      schema.at("type").to_string() == "object" &&
      !schema.defines("additionalProperties") &&
      !schema.defines("unevaluatedProperties")) {
    auto open_path{empty_path};
    open_path.push_back(make_path_segment("wildcard", "*"));
    emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows, frame,
             root, visited, next_identifier);
  }
  walk_prefix_items(schema, empty_path, rows, doc_children, frame, root,
                    visited, next_identifier);

  if (schema.is_object() && schema.defines("items") &&
      schema.at("items").is_object() && !schema.defines("prefixItems")) {
    const auto &items_schema{
        resolve_ref(schema.at("items"), frame, root, visited)};
    if (items_schema.is_object()) {
      auto wildcard_path{sourcemeta::core::JSON::make_array()};
      wildcard_path.push_back(make_path_segment("wildcard", "*"));
      const auto items_row_id{next_identifier};
      emit_row(items_schema, wildcard_path, rows, frame, root, visited,
               next_identifier);
      if (items_schema.defines("type") && items_schema.at("type").is_string() &&
          items_schema.at("type").to_string() == "object") {
        visited.emplace(&items_schema, VisitedEntry{.identifier = items_row_id,
                                                    .path = wildcard_path});
        walk_properties(items_schema, wildcard_path, rows, frame, root, visited,
                        next_identifier);
        walk_pattern_properties(items_schema, wildcard_path, rows, frame, root,
                                visited, next_identifier);
        walk_wildcard_keyword(items_schema, "additionalProperties",
                              wildcard_path, rows, frame, root, visited,
                              next_identifier);
        walk_wildcard_keyword(items_schema, "unevaluatedProperties",
                              wildcard_path, rows, frame, root, visited,
                              next_identifier);
        if (!items_schema.defines("additionalProperties") &&
            !items_schema.defines("unevaluatedProperties")) {
          auto open_path{wildcard_path};
          open_path.push_back(make_path_segment("wildcard", "*"));
          emit_row(sourcemeta::core::JSON{true}, std::move(open_path), rows,
                   frame, root, visited, next_identifier);
        }
        visited.erase(&items_schema);
      }
    }
  }

  walk_branches("anyOf", "Any of", schema, doc_children, frame, root, visited,
                next_identifier);
  walk_branches("oneOf", "One of", schema, doc_children, frame, root, visited,
                next_identifier);
  walk_all_of(schema, rows, doc_children, frame, root, visited,
              next_identifier);
  walk_if_then_else(schema, doc_children, frame, root, visited,
                    next_identifier);
  walk_wildcard_keyword(schema, "unevaluatedProperties", empty_path, rows,
                        frame, root, visited, next_identifier);
  walk_wildcard_keyword(schema, "unevaluatedItems", empty_path, rows, frame,
                        root, visited, next_identifier);

  if (schema.is_object() && schema.defines("contains") &&
      schema.at("contains").is_object()) {
    const auto &contains_schema{schema.at("contains")};
    const auto is_branching{
        contains_schema.defines("anyOf") || contains_schema.defines("oneOf") ||
        contains_schema.defines("allOf") || contains_schema.defines("not") ||
        contains_schema.defines("enum")};
    if (is_branching) {
      walk_branching_subschema("Contains", "matching item", contains_schema,
                               doc_children, frame, root, visited,
                               next_identifier, false);
    }
  }

  if (schema.is_object() && schema.defines("contentSchema") &&
      schema.at("contentSchema").is_object()) {
    const auto &content_schema{schema.at("contentSchema")};
    const auto is_branching{
        content_schema.defines("anyOf") || content_schema.defines("oneOf") ||
        content_schema.defines("allOf") || content_schema.defines("not")};
    if (is_branching) {
      walk_branching_subschema("Decoded content", "decoded", content_schema,
                               doc_children, frame, root, visited,
                               next_identifier, true);
    }
  }

  if (schema.is_object() && schema.defines("propertyNames") &&
      schema.at("propertyNames").is_object()) {
    const auto &names_schema{schema.at("propertyNames")};
    const auto is_branching{
        names_schema.defines("anyOf") || names_schema.defines("oneOf") ||
        names_schema.defines("allOf") || names_schema.defines("not")};
    if (is_branching) {
      walk_branching_subschema("Property names", "key", names_schema,
                               doc_children, frame, root, visited,
                               next_identifier, false);
    }
  }

  if (schema.is_object() && schema.defines("not")) {
    const auto &not_schema{schema.at("not")};
    const auto is_branching{
        not_schema.is_object() &&
        (not_schema.defines("anyOf") || not_schema.defines("oneOf") ||
         not_schema.defines("allOf") || not_schema.defines("not"))};
    const auto has_inline_constraints{!is_branching && not_schema.is_object() &&
                                      !constraints_of(not_schema).empty()};
    if (!has_inline_constraints) {
      walk_branching_subschema("Must NOT match", "value", not_schema,
                               doc_children, frame, root, visited,
                               next_identifier, false);
    }
  }

  assert(!rows.empty() || !doc_children.empty());

  documentation.assign("rows", std::move(rows));
  if (!doc_children.empty()) {
    documentation.assign("children", std::move(doc_children));
  }

  return documentation;
}

} // namespace

auto to_documentation(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaWalker &walker,
                      const sourcemeta::core::SchemaResolver &resolver)
    -> sourcemeta::core::JSON {
  sourcemeta::blaze::SchemaTransformer canonicalizer;
  sourcemeta::blaze::add(canonicalizer,
                         sourcemeta::blaze::AlterSchemaMode::Canonicalizer);
  sourcemeta::core::JSON canonical{schema};
  [[maybe_unused]] const auto canonicalized{canonicalizer.apply(
      canonical, walker, resolver,
      [](const auto &, const auto, const auto, const auto &,
         [[maybe_unused]] const auto applied) { assert(applied); })};
  assert(canonicalized.first);

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(canonical, walker, resolver);

  VisitedSchemas visited;
  std::size_t next_identifier{0};
  return walk_schema(canonical, true, frame, canonical, visited,
                     next_identifier);
}

} // namespace sourcemeta::blaze
