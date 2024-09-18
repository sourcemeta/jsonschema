#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_HELPERS_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_HELPERS_H_

#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <utility> // std::declval, std::move

namespace sourcemeta::jsontoolkit {

static const SchemaCompilerDynamicContext relative_dynamic_context{
    "", empty_pointer, empty_pointer};

// Instantiate a value-oriented step
template <typename Step>
auto make(const bool report, const SchemaCompilerContext &context,
          const SchemaCompilerSchemaContext &schema_context,
          const SchemaCompilerDynamicContext &dynamic_context,
          // Take the value type from the "type" property of the step struct
          decltype(std::declval<Step>().value) &&value) -> Step {
  return {
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location
          : dynamic_context.base_schema_location.concat(
                {dynamic_context.keyword}),
      dynamic_context.base_instance_location,
      to_uri(schema_context.relative_pointer, schema_context.base).recompose(),
      schema_context.base.recompose(),
      context.uses_dynamic_scopes,
      report,
      std::move(value)};
}

// Instantiate an applicator step
template <typename Step>
auto make(const bool report, const SchemaCompilerContext &context,
          const SchemaCompilerSchemaContext &schema_context,
          const SchemaCompilerDynamicContext &dynamic_context,
          // Take the value type from the "value" property of the step struct
          decltype(std::declval<Step>().value) &&value,
          SchemaCompilerTemplate &&children) -> Step {
  return {
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location
          : dynamic_context.base_schema_location.concat(
                {dynamic_context.keyword}),
      dynamic_context.base_instance_location,
      to_uri(schema_context.relative_pointer, schema_context.base).recompose(),
      schema_context.base.recompose(),
      context.uses_dynamic_scopes,
      report,
      std::move(value),
      std::move(children)};
}

template <typename Type, typename Step>
auto unroll(const SchemaCompilerDynamicContext &dynamic_context,
            const Step &step,
            const Pointer &base_instance_location = empty_pointer) -> Type {
  assert(std::holds_alternative<Type>(step));
  return {dynamic_context.keyword.empty()
              ? std::get<Type>(step).relative_schema_location
              : dynamic_context.base_schema_location
                    .concat({dynamic_context.keyword})
                    .concat(std::get<Type>(step).relative_schema_location),
          base_instance_location.concat(
              std::get<Type>(step).relative_instance_location),
          std::get<Type>(step).keyword_location,
          std::get<Type>(step).schema_resource,
          std::get<Type>(step).dynamic,
          std::get<Type>(step).report,
          std::get<Type>(step).value};
}

} // namespace sourcemeta::jsontoolkit

#endif
