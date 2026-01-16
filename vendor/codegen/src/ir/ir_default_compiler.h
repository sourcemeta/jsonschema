#ifndef SOURCEMETA_CODEGEN_IR_DEFAULT_COMPILER_H_
#define SOURCEMETA_CODEGEN_IR_DEFAULT_COMPILER_H_

#include <sourcemeta/codegen/ir.h>

#include <sourcemeta/core/jsonschema.h>

#include <cassert>       // assert
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set

// We do not check vocabularies here because the canonicaliser ensures
// we never get an official keyword when its vocabulary is not present
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ONLY_WHITELIST_KEYWORDS(schema, subschema, pointer, ...)               \
  {                                                                            \
    static const std::unordered_set<std::string_view> allowed{__VA_ARGS__};    \
    for (const auto &entry : (subschema).as_object()) {                        \
      if (!allowed.contains(entry.first)) {                                    \
        throw sourcemeta::codegen::UnsupportedKeywordError(                    \
            (schema), (pointer), entry.first,                                  \
            "Unsupported keyword in subschema");                               \
      }                                                                        \
    }                                                                          \
  }

namespace sourcemeta::codegen {

auto handle_impossible(const sourcemeta::core::JSON &,
                       const sourcemeta::core::SchemaFrame &frame,
                       const sourcemeta::core::SchemaFrame::Location &location,
                       const sourcemeta::core::Vocabularies &,
                       const sourcemeta::core::SchemaResolver &,
                       const sourcemeta::core::JSON &) -> IRImpossible {
  return IRImpossible{
      {.pointer = sourcemeta::core::to_pointer(location.pointer),
       .symbol = symbol(frame, location)}};
}

auto handle_string(const sourcemeta::core::JSON &schema,
                   const sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaFrame::Location &location,
                   const sourcemeta::core::Vocabularies &,
                   const sourcemeta::core::SchemaResolver &,
                   const sourcemeta::core::JSON &subschema) -> IRScalar {
  ONLY_WHITELIST_KEYWORDS(schema, subschema, location.pointer,
                          {"$schema", "$id", "$anchor", "$defs", "$vocabulary",
                           "type", "minLength", "maxLength", "pattern",
                           "format"});
  return IRScalar{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                   .symbol = symbol(frame, location)},
                  IRScalarType::String};
}

auto handle_object(const sourcemeta::core::JSON &schema,
                   const sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaFrame::Location &location,
                   const sourcemeta::core::Vocabularies &,
                   const sourcemeta::core::SchemaResolver &,
                   const sourcemeta::core::JSON &subschema) -> IRObject {
  ONLY_WHITELIST_KEYWORDS(
      schema, subschema, location.pointer,
      {"$defs", "$schema", "$id", "$anchor", "$vocabulary", "type",
       "properties", "required",
       // Note that most programming languages CANNOT represent the idea
       // of additional properties, mainly if they differ from the types of the
       // other properties. Therefore, we whitelist this, but we consider it to
       // be the responsability of the validator
       "additionalProperties", "minProperties", "maxProperties",
       "propertyNames"});

  std::vector<std::pair<sourcemeta::core::JSON::String, IRObjectValue>> members;

  // Guaranteed by canonicalisation
  assert(subschema.defines("properties"));

  const auto &properties{subschema.at("properties")};

  std::unordered_set<std::string_view> required_set;
  if (subschema.defines("required")) {
    const auto &required{subschema.at("required")};
    for (const auto &item : required.as_array()) {
      // Guaranteed by canonicalisation
      assert(properties.defines(item.to_string()));
      required_set.insert(item.to_string());
    }
  }

  for (const auto &entry : properties.as_object()) {
    auto property_pointer{sourcemeta::core::to_pointer(location.pointer)};
    property_pointer.push_back("properties");
    property_pointer.push_back(entry.first);

    const auto property_location{
        frame.traverse(sourcemeta::core::to_weak_pointer(property_pointer))};
    assert(property_location.has_value());

    IRObjectValue member_value{
        {.pointer = std::move(property_pointer),
         .symbol = symbol(frame, property_location.value().get())},
        required_set.contains(entry.first),
        false};

    members.emplace_back(entry.first, std::move(member_value));
  }

  std::variant<bool, IRType> additional{true};
  if (subschema.defines("additionalProperties")) {
    const auto &additional_schema{subschema.at("additionalProperties")};
    if (additional_schema.is_boolean()) {
      additional = additional_schema.to_boolean();
    } else {
      auto additional_pointer{sourcemeta::core::to_pointer(location.pointer)};
      additional_pointer.push_back("additionalProperties");

      const auto additional_location{frame.traverse(
          sourcemeta::core::to_weak_pointer(additional_pointer))};
      assert(additional_location.has_value());

      additional =
          IRType{.pointer = std::move(additional_pointer),
                 .symbol = symbol(frame, additional_location.value().get())};
    }
  }

  return IRObject{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                   .symbol = symbol(frame, location)},
                  std::move(members),
                  std::move(additional)};
}

auto handle_integer(const sourcemeta::core::JSON &schema,
                    const sourcemeta::core::SchemaFrame &frame,
                    const sourcemeta::core::SchemaFrame::Location &location,
                    const sourcemeta::core::Vocabularies &,
                    const sourcemeta::core::SchemaResolver &,
                    const sourcemeta::core::JSON &subschema) -> IRScalar {
  ONLY_WHITELIST_KEYWORDS(schema, subschema, location.pointer,
                          {"$schema", "$id", "$anchor", "$defs", "$vocabulary",
                           "type", "minimum", "maximum", "exclusiveMinimum",
                           "exclusiveMaximum", "multipleOf"});
  return IRScalar{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                   .symbol = symbol(frame, location)},
                  IRScalarType::Integer};
}

auto handle_number(const sourcemeta::core::JSON &schema,
                   const sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaFrame::Location &location,
                   const sourcemeta::core::Vocabularies &,
                   const sourcemeta::core::SchemaResolver &,
                   const sourcemeta::core::JSON &subschema) -> IRScalar {
  ONLY_WHITELIST_KEYWORDS(schema, subschema, location.pointer,
                          {"$schema", "$id", "$anchor", "$defs", "$vocabulary",
                           "type", "minimum", "maximum", "exclusiveMinimum",
                           "exclusiveMaximum", "multipleOf"});
  return IRScalar{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                   .symbol = symbol(frame, location)},
                  IRScalarType::Number};
}

auto handle_array(const sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaFrame &frame,
                  const sourcemeta::core::SchemaFrame::Location &location,
                  const sourcemeta::core::Vocabularies &vocabularies,
                  const sourcemeta::core::SchemaResolver &,
                  const sourcemeta::core::JSON &subschema) -> IREntity {
  ONLY_WHITELIST_KEYWORDS(schema, subschema, location.pointer,
                          {"$schema", "$id", "$anchor", "$defs", "$vocabulary",
                           "type", "items", "minItems", "maxItems",
                           "uniqueItems", "contains", "minContains",
                           "maxContains", "additionalItems", "prefixItems"});

  using Vocabularies = sourcemeta::core::Vocabularies;

  if (vocabularies.contains(
          Vocabularies::Known::JSON_Schema_2020_12_Applicator) &&
      subschema.defines("prefixItems")) {
    const auto &prefix_items{subschema.at("prefixItems")};
    assert(prefix_items.is_array());

    std::vector<IRType> tuple_items;
    for (std::size_t index = 0; index < prefix_items.size(); ++index) {
      auto item_pointer{sourcemeta::core::to_pointer(location.pointer)};
      item_pointer.push_back("prefixItems");
      item_pointer.push_back(index);

      const auto item_location{
          frame.traverse(sourcemeta::core::to_weak_pointer(item_pointer))};
      assert(item_location.has_value());

      tuple_items.push_back(
          {.pointer = std::move(item_pointer),
           .symbol = symbol(frame, item_location.value().get())});
    }

    std::optional<IRType> additional{std::nullopt};
    if (subschema.defines("items")) {
      auto additional_pointer{sourcemeta::core::to_pointer(location.pointer)};
      additional_pointer.push_back("items");

      const auto additional_location{frame.traverse(
          sourcemeta::core::to_weak_pointer(additional_pointer))};
      assert(additional_location.has_value());

      additional =
          IRType{.pointer = std::move(additional_pointer),
                 .symbol = symbol(frame, additional_location.value().get())};
    }

    return IRTuple{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                    .symbol = symbol(frame, location)},
                   std::move(tuple_items),
                   std::move(additional)};
  }

  if (vocabularies.contains_any(
          {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
           Vocabularies::Known::JSON_Schema_Draft_7,
           Vocabularies::Known::JSON_Schema_Draft_6,
           Vocabularies::Known::JSON_Schema_Draft_4,
           Vocabularies::Known::JSON_Schema_Draft_3}) &&
      subschema.defines("items") && subschema.at("items").is_array()) {
    const auto &items_array{subschema.at("items")};

    std::vector<IRType> tuple_items;
    for (std::size_t index = 0; index < items_array.size(); ++index) {
      auto item_pointer{sourcemeta::core::to_pointer(location.pointer)};
      item_pointer.push_back("items");
      item_pointer.push_back(index);

      const auto item_location{
          frame.traverse(sourcemeta::core::to_weak_pointer(item_pointer))};
      assert(item_location.has_value());

      tuple_items.push_back(
          {.pointer = std::move(item_pointer),
           .symbol = symbol(frame, item_location.value().get())});
    }

    std::optional<IRType> additional{std::nullopt};
    if (subschema.defines("additionalItems")) {
      auto additional_pointer{sourcemeta::core::to_pointer(location.pointer)};
      additional_pointer.push_back("additionalItems");

      const auto additional_location{frame.traverse(
          sourcemeta::core::to_weak_pointer(additional_pointer))};
      assert(additional_location.has_value());

      additional =
          IRType{.pointer = std::move(additional_pointer),
                 .symbol = symbol(frame, additional_location.value().get())};
    }

    return IRTuple{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                    .symbol = symbol(frame, location)},
                   std::move(tuple_items),
                   std::move(additional)};
  }

  std::optional<IRType> items_type{std::nullopt};
  if (subschema.defines("items")) {
    auto items_pointer{sourcemeta::core::to_pointer(location.pointer)};
    items_pointer.push_back("items");

    const auto items_location{
        frame.traverse(sourcemeta::core::to_weak_pointer(items_pointer))};
    assert(items_location.has_value());

    items_type = IRType{.pointer = std::move(items_pointer),
                        .symbol = symbol(frame, items_location.value().get())};
  }

  return IRArray{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                  .symbol = symbol(frame, location)},
                 std::move(items_type)};
}

auto handle_enum(const sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaFrame &frame,
                 const sourcemeta::core::SchemaFrame::Location &location,
                 const sourcemeta::core::Vocabularies &,
                 const sourcemeta::core::SchemaResolver &,
                 const sourcemeta::core::JSON &subschema) -> IREntity {
  ONLY_WHITELIST_KEYWORDS(
      schema, subschema, location.pointer,
      {"$schema", "$id", "$anchor", "$defs", "$vocabulary", "enum"});
  const auto &enum_json{subschema.at("enum")};

  // Boolean and null special cases
  if (enum_json.size() == 1 && enum_json.at(0).is_null()) {
    return IRScalar{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                     .symbol = symbol(frame, location)},
                    IRScalarType::Null};
  } else if (enum_json.size() == 2) {
    const auto &first{enum_json.at(0)};
    const auto &second{enum_json.at(1)};
    if ((first.is_boolean() && second.is_boolean()) &&
        (first.to_boolean() != second.to_boolean())) {
      return IRScalar{
          {.pointer = sourcemeta::core::to_pointer(location.pointer),
           .symbol = symbol(frame, location)},
          IRScalarType::Boolean};
    }
  }

  std::vector<sourcemeta::core::JSON> values{enum_json.as_array().cbegin(),
                                             enum_json.as_array().cend()};
  return IREnumeration{
      {.pointer = sourcemeta::core::to_pointer(location.pointer),
       .symbol = symbol(frame, location)},
      std::move(values)};
}

auto handle_anyof(const sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaFrame &frame,
                  const sourcemeta::core::SchemaFrame::Location &location,
                  const sourcemeta::core::Vocabularies &,
                  const sourcemeta::core::SchemaResolver &,
                  const sourcemeta::core::JSON &subschema) -> IREntity {
  ONLY_WHITELIST_KEYWORDS(
      schema, subschema, location.pointer,
      {"$schema", "$id", "$anchor", "$defs", "$vocabulary", "anyOf"});

  const auto &any_of{subschema.at("anyOf")};
  assert(any_of.is_array());
  assert(any_of.size() >= 2);

  std::vector<IRType> branches;
  for (std::size_t index = 0; index < any_of.size(); ++index) {
    auto branch_pointer{sourcemeta::core::to_pointer(location.pointer)};
    branch_pointer.push_back("anyOf");
    branch_pointer.push_back(index);

    const auto branch_location{
        frame.traverse(sourcemeta::core::to_weak_pointer(branch_pointer))};
    assert(branch_location.has_value());

    branches.push_back(
        {.pointer = std::move(branch_pointer),
         .symbol = symbol(frame, branch_location.value().get())});
  }

  return IRUnion{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                  .symbol = symbol(frame, location)},
                 std::move(branches)};
}

auto handle_oneof(const sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaFrame &frame,
                  const sourcemeta::core::SchemaFrame::Location &location,
                  const sourcemeta::core::Vocabularies &,
                  const sourcemeta::core::SchemaResolver &,
                  const sourcemeta::core::JSON &subschema) -> IREntity {
  ONLY_WHITELIST_KEYWORDS(
      schema, subschema, location.pointer,
      {"$schema", "$id", "$anchor", "$defs", "$vocabulary", "oneOf"});

  const auto &one_of{subschema.at("oneOf")};
  assert(one_of.is_array());
  assert(one_of.size() >= 2);

  std::vector<IRType> branches;
  for (std::size_t index = 0; index < one_of.size(); ++index) {
    auto branch_pointer{sourcemeta::core::to_pointer(location.pointer)};
    branch_pointer.push_back("oneOf");
    branch_pointer.push_back(index);

    const auto branch_location{
        frame.traverse(sourcemeta::core::to_weak_pointer(branch_pointer))};
    assert(branch_location.has_value());

    branches.push_back(
        {.pointer = std::move(branch_pointer),
         .symbol = symbol(frame, branch_location.value().get())});
  }

  return IRUnion{{.pointer = sourcemeta::core::to_pointer(location.pointer),
                  .symbol = symbol(frame, location)},
                 std::move(branches)};
}

auto handle_ref(const sourcemeta::core::JSON &schema,
                const sourcemeta::core::SchemaFrame &frame,
                const sourcemeta::core::SchemaFrame::Location &location,
                const sourcemeta::core::Vocabularies &,
                const sourcemeta::core::SchemaResolver &,
                const sourcemeta::core::JSON &subschema) -> IREntity {
  ONLY_WHITELIST_KEYWORDS(
      schema, subschema, location.pointer,
      {"$schema", "$id", "$anchor", "$defs", "$vocabulary", "$ref"});

  auto ref_pointer{sourcemeta::core::to_pointer(location.pointer)};
  ref_pointer.push_back("$ref");
  const auto ref_weak_pointer{sourcemeta::core::to_weak_pointer(ref_pointer)};

  const auto &references{frame.references()};
  const auto reference{references.find(
      {sourcemeta::core::SchemaReferenceType::Static, ref_weak_pointer})};
  assert(reference != references.cend());

  const auto &destination{reference->second.destination};
  const auto target{frame.traverse(destination)};
  if (!target.has_value()) {
    throw UnexpectedSchemaError(schema, location.pointer,
                                "Could not resolve reference destination");
  }

  const auto &target_location{target.value().get()};

  return IRReference{
      {.pointer = sourcemeta::core::to_pointer(location.pointer),
       .symbol = symbol(frame, location)},
      {.pointer = sourcemeta::core::to_pointer(target_location.pointer),
       .symbol = symbol(frame, target_location)}};
}

auto default_compiler(const sourcemeta::core::JSON &schema,
                      const sourcemeta::core::SchemaFrame &frame,
                      const sourcemeta::core::SchemaFrame::Location &location,
                      const sourcemeta::core::SchemaResolver &resolver,
                      const sourcemeta::core::JSON &subschema) -> IREntity {
  const auto vocabularies{frame.vocabularies(location, resolver)};
  assert(!vocabularies.empty());

  // Be strict with vocabulary support
  using Vocabularies = sourcemeta::core::Vocabularies;
  static const std::unordered_set<Vocabularies::URI> supported{
      Vocabularies::Known::JSON_Schema_2020_12_Core,
      Vocabularies::Known::JSON_Schema_2020_12_Applicator,
      Vocabularies::Known::JSON_Schema_2020_12_Validation,
      Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
      Vocabularies::Known::JSON_Schema_2020_12_Content,
      Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
      Vocabularies::Known::JSON_Schema_2020_12_Format_Annotation,
      Vocabularies::Known::JSON_Schema_2020_12_Format_Assertion,
      Vocabularies::Known::JSON_Schema_2019_09_Core,
      Vocabularies::Known::JSON_Schema_2019_09_Applicator,
      Vocabularies::Known::JSON_Schema_2019_09_Validation,
      Vocabularies::Known::JSON_Schema_2019_09_Content,
      Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
      Vocabularies::Known::JSON_Schema_2019_09_Format,
      Vocabularies::Known::JSON_Schema_Draft_7,
      Vocabularies::Known::JSON_Schema_Draft_6,
      Vocabularies::Known::JSON_Schema_Draft_4};
  vocabularies.throw_if_any_unsupported(supported,
                                        "Unsupported required vocabulary");

  // The canonicaliser ensures that every subschema schema is only in one of the
  // following shapes

  if (subschema.is_boolean()) {
    assert(!subschema.to_boolean());
    return handle_impossible(schema, frame, location, vocabularies, resolver,
                             subschema);
  } else if (subschema.defines("type")) {
    const auto &type_value{subschema.at("type")};
    if (!type_value.is_string()) {
      throw UnsupportedKeywordValueError(schema, location.pointer, "type",
                                         "Expected a string value");
    }

    const auto &type_string{type_value.to_string()};

    // The canonicaliser transforms any other type
    if (type_string == "string") {
      return handle_string(schema, frame, location, vocabularies, resolver,
                           subschema);
    } else if (type_string == "object") {
      return handle_object(schema, frame, location, vocabularies, resolver,
                           subschema);
    } else if (type_string == "integer") {
      return handle_integer(schema, frame, location, vocabularies, resolver,
                            subschema);
    } else if (type_string == "number") {
      return handle_number(schema, frame, location, vocabularies, resolver,
                           subschema);
    } else if (type_string == "array") {
      return handle_array(schema, frame, location, vocabularies, resolver,
                          subschema);
    } else {
      throw UnsupportedKeywordValueError(schema, location.pointer, "type",
                                         "Unsupported type value");
    }
  } else if (subschema.defines("enum")) {
    return handle_enum(schema, frame, location, vocabularies, resolver,
                       subschema);
  } else if (subschema.defines("anyOf")) {
    return handle_anyof(schema, frame, location, vocabularies, resolver,
                        subschema);
    // This is usually a good enough approximation. We usually can't check that
    // the other types DO NOT match, but that is in a way a validation concern
  } else if (subschema.defines("oneOf")) {
    return handle_oneof(schema, frame, location, vocabularies, resolver,
                        subschema);
  } else if (subschema.defines("$ref")) {
    return handle_ref(schema, frame, location, vocabularies, resolver,
                      subschema);
  } else {
    throw UnexpectedSchemaError(schema, location.pointer, "Unsupported schema");
  }
}

} // namespace sourcemeta::codegen

#endif
