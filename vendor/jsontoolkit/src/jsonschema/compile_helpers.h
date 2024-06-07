#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_HELPERS_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_HELPERS_H_

#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <utility> // std::declval, std::move

namespace {

template <typename T>
auto concat_set(const std::set<T> &current, const T value) -> std::set<T> {
  auto result{current};
  result.insert(value);
  return result;
}

} // namespace

namespace sourcemeta::jsontoolkit {

inline auto applicate(const SchemaCompilerContext &context,
                      const std::optional<std::size_t> &label = std::nullopt)
    -> SchemaCompilerContext {
  return {"",
          context.schema,
          context.vocabularies,
          context.value,
          context.root,
          context.base,
          context.relative_pointer,
          empty_pointer,
          empty_pointer,
          label.has_value() ? concat_set(context.labels, label.value())
                            : context.labels,
          context.frame,
          context.references,
          context.walker,
          context.resolver,
          context.compiler,
          context.default_dialect};
}

inline auto
keyword_location(const SchemaCompilerContext &context) -> std::string {
  return to_uri(context.relative_pointer, context.base).recompose();
}

inline auto
relative_schema_location(const SchemaCompilerContext &context) -> Pointer {
  return context.keyword.empty()
             ? context.base_schema_location
             : context.base_schema_location.concat({context.keyword});
}

// Instantiate a value-oriented step
template <typename Step>
auto make(const SchemaCompilerContext &context,
          // Take the value type from the "type" property of the step struct
          decltype(std::declval<Step>().value) &&value,
          SchemaCompilerTemplate &&condition,
          const SchemaCompilerTargetType target_type,
          const std::optional<Pointer> &target_location = std::nullopt)
    -> Step {
  return {{target_type, target_location.value_or(empty_pointer)},
          relative_schema_location(context),
          context.base_instance_location,
          keyword_location(context),
          std::move(value),
          std::move(condition)};
}

// Instantiate an applicator step
template <typename Step>
auto make(const SchemaCompilerContext &context,
          // Take the value type from the "type" property of the step struct
          decltype(std::declval<Step>().value) &&value,
          SchemaCompilerTemplate &&children,
          SchemaCompilerTemplate &&condition) -> Step {
  return {{SchemaCompilerTargetType::Instance, empty_pointer},
          relative_schema_location(context),
          context.base_instance_location,
          keyword_location(context),
          std::move(value),
          std::move(children),
          std::move(condition)};
}

// Instantiate a control step
template <typename Step>
auto make(const SchemaCompilerContext &context, const std::size_t id,
          SchemaCompilerTemplate &&children) -> Step {
  return {relative_schema_location(context), context.base_instance_location,
          keyword_location(context), id, std::move(children)};
}

inline auto type_condition(const SchemaCompilerContext &context,
                           const JSON::Type type) -> SchemaCompilerTemplate {
  // As an optimization
  if (context.schema.is_object() && context.schema.defines("type") &&
      context.schema.at("type").is_string()) {
    const auto &type_string{context.schema.at("type").to_string()};
    if (type == JSON::Type::Null && type_string == "null") {
      return {};
    } else if (type == JSON::Type::Boolean && type_string == "boolean") {
      return {};
    } else if (type == JSON::Type::Integer && type_string == "integer") {
      return {};
    } else if (type == JSON::Type::String && type_string == "string") {
      return {};
    } else if (type == JSON::Type::Array && type_string == "array") {
      return {};
    } else if (type == JSON::Type::Object && type_string == "object") {
      return {};
    }
  }

  return {make<SchemaCompilerAssertionTypeStrict>(
      applicate(context), type, {}, SchemaCompilerTargetType::Instance)};
}

} // namespace sourcemeta::jsontoolkit

#endif
