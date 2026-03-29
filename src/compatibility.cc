#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>   // std::includes, std::set_difference, std::sort
#include <iterator>    // std::back_inserter
#include <optional>    // std::optional
#include <set>         // std::set
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

#include "compatibility.h"

namespace {

using JSON = sourcemeta::core::JSON;
using JSONType = sourcemeta::core::JSON::Type;
using TypeSet = sourcemeta::core::JSON::TypeSet;

auto stringify_json(const JSON &value) -> std::string {
  std::ostringstream stream;
  sourcemeta::core::stringify(value, stream);
  return stream.str();
}

auto join_path(const std::string &path, const std::string &property)
    -> std::string {
  if (path.empty()) {
    return property;
  }

  return path + "." + property;
}

auto append_items_path(const std::string &path) -> std::string {
  if (path.empty()) {
    return "[]";
  }

  return path + "[]";
}

auto property_label(const std::string &path) -> std::string {
  return "property \"" + (path.empty() ? std::string{"<root>"} : path) + "\"";
}

auto schema_label(const std::string &path) -> std::string {
  if (path.empty()) {
    return "schema";
  }

  return property_label(path);
}

auto enum_label(const std::string &path) -> std::string {
  return "\"" + (path.empty() ? std::string{"<root>"} : path) + "\"";
}

auto typeset_empty(const TypeSet &types) -> bool { return types.none(); }

auto typeset_difference(const TypeSet &left, const TypeSet &right) -> TypeSet {
  return left & (~right);
}

auto format_typeset(const TypeSet &types) -> std::string {
  std::vector<std::string> labels;

  if (types.test(static_cast<std::size_t>(JSONType::Null))) {
    labels.emplace_back("null");
  }

  if (types.test(static_cast<std::size_t>(JSONType::Boolean))) {
    labels.emplace_back("boolean");
  }

  if (types.test(static_cast<std::size_t>(JSONType::Object))) {
    labels.emplace_back("object");
  }

  if (types.test(static_cast<std::size_t>(JSONType::Array))) {
    labels.emplace_back("array");
  }

  const auto has_integer{
      types.test(static_cast<std::size_t>(JSONType::Integer))};
  const auto has_real{types.test(static_cast<std::size_t>(JSONType::Real))};
  if (has_integer && has_real) {
    labels.emplace_back("number");
  } else if (has_integer) {
    labels.emplace_back("integer");
  } else if (has_real) {
    labels.emplace_back("number");
  }

  if (types.test(static_cast<std::size_t>(JSONType::String))) {
    labels.emplace_back("string");
  }

  std::ostringstream stream;
  if (labels.size() == 1) {
    stream << "\"" << labels.front() << "\"";
  } else {
    stream << "[";
    for (auto iterator = labels.cbegin(); iterator != labels.cend();
         ++iterator) {
      if (iterator != labels.cbegin()) {
        stream << ", ";
      }

      stream << "\"" << *iterator << "\"";
    }
    stream << "]";
  }

  return stream.str();
}

auto required_set(const JSON &schema) -> std::set<std::string> {
  std::set<std::string> result;
  if (!schema.is_object() || !schema.defines("required")) {
    return result;
  }

  const auto &required{schema.at("required")};
  if (!required.is_array()) {
    return result;
  }

  for (const auto &entry : required.as_array()) {
    if (entry.is_string()) {
      result.emplace(entry.to_string());
    }
  }

  return result;
}

auto enum_values(const JSON &schema) -> std::vector<JSON> {
  std::vector<JSON> result;
  if (!schema.is_object() || !schema.defines("enum")) {
    return result;
  }

  const auto &enumeration{schema.at("enum")};
  if (!enumeration.is_array()) {
    return result;
  }

  for (const auto &entry : enumeration.as_array()) {
    result.push_back(entry);
  }

  return result;
}

auto sorted_unique_enum_values(const JSON &schema) -> std::vector<JSON> {
  auto result{enum_values(schema)};
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}

auto parse_types(const JSON &schema) -> std::optional<TypeSet> {
  if (!schema.is_object() || !schema.defines("type")) {
    return std::nullopt;
  }

  return sourcemeta::core::parse_schema_type(schema.at("type"));
}

auto optional_object_keyword(const JSON &schema, const std::string &keyword)
    -> const JSON * {
  if (!schema.is_object() || !schema.defines(keyword)) {
    return nullptr;
  }

  return &schema.at(keyword);
}

auto optional_number_keyword(const JSON &schema, const std::string &keyword)
    -> std::optional<double> {
  const auto *value{optional_object_keyword(schema, keyword)};
  if (value == nullptr) {
    return std::nullopt;
  }

  if (value->is_integer()) {
    return static_cast<double>(value->to_integer());
  }

  if (value->is_real()) {
    return value->to_real();
  }

  return std::nullopt;
}

auto compare_numeric_keyword(
    const std::string &path, const JSON &old_schema, const JSON &new_schema,
    const std::string &keyword, const bool warn_when_decreased,
    sourcemeta::jsonschema::CompatibilityReport &report) -> void {
  const auto old_value{optional_number_keyword(old_schema, keyword)};
  const auto new_value{optional_number_keyword(new_schema, keyword)};
  if (!old_value.has_value() || !new_value.has_value()) {
    return;
  }

  const bool warning{warn_when_decreased
                         ? new_value.value() < old_value.value()
                         : new_value.value() > old_value.value()};
  if (!warning) {
    return;
  }

  std::ostringstream message;
  message << keyword
          << (warn_when_decreased ? " reduced from " : " increased from ")
          << old_value.value() << " to " << new_value.value();
  if (!path.empty()) {
    message << " for " << property_label(path);
  }

  report.warnings.push_back(
      {sourcemeta::jsonschema::CompatibilitySeverity::Warning, keyword, path,
       message.str()});
}

auto compare_schema(const JSON &old_schema, const JSON &new_schema,
                    const std::string &path,
                    sourcemeta::jsonschema::CompatibilityReport &report)
    -> void {
  if (old_schema.is_boolean() || new_schema.is_boolean()) {
    if (old_schema == new_schema) {
      return;
    }

    if (old_schema.is_boolean() && old_schema.to_boolean() &&
        new_schema.is_boolean() && !new_schema.to_boolean()) {
      report.breaking.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Breaking,
           "boolean_schema", path,
           schema_label(path) + " now rejects every instance"});
    } else if (old_schema.is_boolean() && !old_schema.to_boolean() &&
               new_schema.is_boolean() && new_schema.to_boolean()) {
      report.safe.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Safe,
           "boolean_schema", path,
           schema_label(path) + " now accepts every instance"});
    }

    return;
  }

  if (!old_schema.is_object() || !new_schema.is_object()) {
    return;
  }

  const auto old_required{required_set(old_schema)};
  const auto new_required{required_set(new_schema)};
  for (const auto &property : new_required) {
    if (!old_required.contains(property)) {
      const auto property_path{join_path(path, property)};
      report.breaking.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Breaking,
           "required_added", property_path,
           property_label(property_path) + " is now required"});
    }
  }

  for (const auto &property : old_required) {
    if (!new_required.contains(property)) {
      const auto property_path{join_path(path, property)};
      report.safe.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Safe,
           "required_removed", property_path,
           property_label(property_path) + " is no longer required"});
    }
  }

  const auto old_enum{sorted_unique_enum_values(old_schema)};
  const auto new_enum{sorted_unique_enum_values(new_schema)};
  if (!old_enum.empty() || !new_enum.empty()) {
    std::vector<JSON> removed;
    std::vector<JSON> added;
    std::set_difference(old_enum.cbegin(), old_enum.cend(), new_enum.cbegin(),
                        new_enum.cend(), std::back_inserter(removed));
    std::set_difference(new_enum.cbegin(), new_enum.cend(), old_enum.cbegin(),
                        old_enum.cend(), std::back_inserter(added));

    for (const auto &value : removed) {
      report.breaking.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Breaking,
           "enum_removed", path,
           "enum " + enum_label(path) + " removed value " +
               stringify_json(value)});
    }

    for (const auto &value : added) {
      report.safe.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Safe, "enum_added",
           path,
           "enum " + enum_label(path) + " added value " +
               stringify_json(value)});
    }
  }

  const auto old_types{parse_types(old_schema)};
  const auto new_types{parse_types(new_schema)};
  if (old_types.has_value() && new_types.has_value() &&
      old_types.value() != new_types.value()) {
    const auto removed{
        typeset_difference(old_types.value(), new_types.value())};
    const auto added{typeset_difference(new_types.value(), old_types.value())};

    if (!typeset_empty(removed)) {
      const auto kind{typeset_empty(added) ? "type_narrowing" : "type_change"};
      const auto verb{typeset_empty(added) ? "narrowed" : "changed"};
      report.breaking.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Breaking, kind, path,
           schema_label(path) + " " + verb + " type from " +
               format_typeset(old_types.value()) + " to " +
               format_typeset(new_types.value())});
    } else if (!typeset_empty(added)) {
      report.safe.push_back(
          {sourcemeta::jsonschema::CompatibilitySeverity::Safe, "type_widening",
           path,
           schema_label(path) + " widened type from " +
               format_typeset(old_types.value()) + " to " +
               format_typeset(new_types.value())});
    }
  }

  compare_numeric_keyword(path, old_schema, new_schema, "maxLength", true,
                          report);
  compare_numeric_keyword(path, old_schema, new_schema, "minLength", false,
                          report);
  compare_numeric_keyword(path, old_schema, new_schema, "maximum", true,
                          report);
  compare_numeric_keyword(path, old_schema, new_schema, "minimum", false,
                          report);

  const JSON *const old_properties{
      optional_object_keyword(old_schema, "properties")};
  const JSON *const new_properties{
      optional_object_keyword(new_schema, "properties")};
  if (old_properties != nullptr && old_properties->is_object() &&
      new_properties != nullptr && new_properties->is_object()) {
    for (const auto &entry : old_properties->as_object()) {
      const auto &name{entry.first};
      const auto &schema{entry.second};
      if (new_properties->defines(name)) {
        compare_schema(schema, new_properties->at(name), join_path(path, name),
                       report);
      }
    }
  }

  if (new_properties != nullptr && new_properties->is_object()) {
    for (const auto &entry : new_properties->as_object()) {
      const auto &name{entry.first};
      const auto property_path{join_path(path, name)};
      const auto existed_before{old_properties != nullptr &&
                                old_properties->is_object() &&
                                old_properties->defines(name)};
      if (!existed_before && !new_required.contains(name)) {
        report.safe.push_back(
            {sourcemeta::jsonschema::CompatibilitySeverity::Safe,
             "optional_property_added", property_path,
             "added optional property \"" + property_path + "\""});
      }
    }
  }

  const JSON *const old_items{optional_object_keyword(old_schema, "items")};
  const JSON *const new_items{optional_object_keyword(new_schema, "items")};
  if (old_items != nullptr && new_items != nullptr &&
      sourcemeta::core::is_schema(*old_items) &&
      sourcemeta::core::is_schema(*new_items)) {
    compare_schema(*old_items, *new_items, append_items_path(path), report);
  }
}

auto changes_to_json(
    const std::vector<sourcemeta::jsonschema::CompatibilityChange> &changes)
    -> JSON {
  auto result{JSON::make_array()};
  for (const auto &change : changes) {
    auto entry{JSON::make_object()};
    entry.assign("kind", JSON{change.kind});
    entry.assign("path", JSON{change.path});
    entry.assign("message", JSON{change.message});
    result.push_back(std::move(entry));
  }

  return result;
}

} // namespace

auto sourcemeta::jsonschema::CompatibilityReport::empty() const noexcept
    -> bool {
  return this->breaking.empty() && this->warnings.empty() && this->safe.empty();
}

auto sourcemeta::jsonschema::CompatibilityReport::has_breaking_changes()
    const noexcept -> bool {
  return !this->breaking.empty();
}

auto sourcemeta::jsonschema::CompatibilityReport::to_json() const -> JSON {
  auto result{JSON::make_object()};
  result.assign("breaking", changes_to_json(this->breaking));
  result.assign("warnings", changes_to_json(this->warnings));
  result.assign("safe", changes_to_json(this->safe));

  auto summary{JSON::make_object()};
  summary.assign("breaking",
                 JSON{static_cast<std::size_t>(this->breaking.size())});
  summary.assign("warnings",
                 JSON{static_cast<std::size_t>(this->warnings.size())});
  summary.assign("safe", JSON{static_cast<std::size_t>(this->safe.size())});
  summary.assign("compatible", JSON{!this->has_breaking_changes()});
  result.assign("summary", std::move(summary));

  return result;
}

auto sourcemeta::jsonschema::CompatibilityChecker::compare(
    const JSON &old_schema, const JSON &new_schema) const
    -> CompatibilityReport {
  CompatibilityReport report;
  compare_schema(old_schema, new_schema, "", report);
  return report;
}
