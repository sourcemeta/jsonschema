#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::all_of

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft6_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_null(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_null()) {
        return {};
      }

      return {
          make<AssertionTypeStrict>(context, schema_context, dynamic_context,
                                    sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_boolean(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_boolean()) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeObjectBounded>(context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_object(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_object()) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeArrayBounded>(context, schema_context,
                                                dynamic_context,
                                                {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_array(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_array()) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_number(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_number()) {
        return {};
      }

      return {make<AssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) {
                        return value.is_integer() || value.is_integer_real();
                      })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          (schema_context.schema.at("const").is_integer() ||
           schema_context.schema.at("const").is_integer_real())) {
        return {};
      }

      return {
          make<AssertionType>(context, schema_context, dynamic_context,
                              sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (context.mode == Mode::FastValidation &&
          (minimum > 0 || maximum.has_value())) {
        return {make<AssertionTypeStringBounded>(context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_string(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_string()) {
        return {};
      }

      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array() &&
             schema_context.schema.at(dynamic_context.keyword).size() == 1 &&
             schema_context.schema.at(dynamic_context.keyword)
                 .front()
                 .is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).front().to_string()};
    if (type == "null") {
      return {
          make<AssertionTypeStrict>(context, schema_context, dynamic_context,
                                    sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {
          make<AssertionType>(context, schema_context, dynamic_context,
                              sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<AssertionTypeStrict>(
          context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<sourcemeta::jsontoolkit::JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<AssertionTypeAny>(context, schema_context, dynamic_context,
                                   std::move(types))};
  }

  return {};
}

auto compiler_draft6_validation_const(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  return {make<AssertionEqual>(
      context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusivemaximum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<AssertionLess>(
      context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusiveminimum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<AssertionGreater>(
      context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_applicator_contains(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  Template children{compile(context, schema_context, relative_dynamic_context,
                            sourcemeta::jsontoolkit::empty_pointer,
                            sourcemeta::jsontoolkit::empty_pointer)};

  if (children.empty()) {
    // We still need to check the instance is not empty
    return {make<AssertionArraySizeGreater>(
        context, schema_context, dynamic_context, ValueUnsignedInteger{0})};
  }

  return {make<LoopContains>(context, schema_context, dynamic_context,
                             ValueRange{1, std::nullopt, false},
                             std::move(children))};
}

auto compiler_draft6_validation_propertynames(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Template children{compile(context, schema_context, relative_dynamic_context,
                            sourcemeta::jsontoolkit::empty_pointer,
                            sourcemeta::jsontoolkit::empty_pointer)};

  if (children.empty()) {
    return {};
  }

  return {make<LoopKeys>(context, schema_context, dynamic_context, ValueNone{},
                         std::move(children))};
}

} // namespace internal
#endif
