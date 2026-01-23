#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>  // std::ranges::find, std::ranges::any_of
#include <cassert>    // assert
#include <functional> // std::cref
#include <iterator>   // std::distance
#include <regex>      // std::regex, std::regex_match, std::smatch
#include <utility>    // std::declval, std::move

namespace sourcemeta::blaze {

// Static keyword strings for use in DynamicContext references
static const sourcemeta::core::JSON::String KEYWORD_EMPTY{};
static const sourcemeta::core::JSON::String KEYWORD_PROPERTIES{"properties"};
static const sourcemeta::core::JSON::String KEYWORD_THEN{"then"};
static const sourcemeta::core::JSON::String KEYWORD_ELSE{"else"};

// Helper to create a single-element WeakPointer from a property name reference
inline auto make_weak_pointer(const std::string &property)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property));
  return result;
}

// Helper to create a two-element WeakPointer from property name and index
inline auto make_weak_pointer(const std::string &property,
                              const std::size_t index)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property));
  result.push_back(index);
  return result;
}

// Helper to create a two-element WeakPointer from two property names
inline auto make_weak_pointer(const std::string &property1,
                              const std::string &property2)
    -> sourcemeta::core::WeakPointer {
  sourcemeta::core::WeakPointer result;
  result.push_back(std::cref(property1));
  result.push_back(std::cref(property2));
  return result;
}

inline auto relative_dynamic_context() -> DynamicContext {
  return {.keyword = KEYWORD_EMPTY,
          .base_schema_location = sourcemeta::core::empty_weak_pointer,
          .base_instance_location = sourcemeta::core::empty_weak_pointer};
}

inline auto schema_resource_id(const std::vector<std::string> &resources,
                               const std::string_view resource) -> std::size_t {
  const auto iterator{std::ranges::find(
      resources, sourcemeta::core::URI::canonicalize(resource))};
  if (iterator == resources.cend()) {
    assert(resource.empty());
    return 0;
  }

  return 1 +
         static_cast<std::size_t>(std::distance(resources.cbegin(), iterator));
}

// Instantiate a value-oriented step with a custom resource
inline auto make_with_resource(const InstructionIndex type,
                               const Context &context,
                               const SchemaContext &schema_context,
                               const DynamicContext &dynamic_context,
                               const Value &value, const std::string &resource)
    -> Instruction {
  const auto schema_location{
      dynamic_context.keyword.empty()
          ? to_pointer(dynamic_context.base_schema_location)
          : to_pointer(dynamic_context.base_schema_location)
                .concat({dynamic_context.keyword})};
  return {.type = type,
          .relative_schema_location = schema_location,
          .relative_instance_location =
              to_pointer(dynamic_context.base_instance_location),
          .keyword_location =
              to_uri(schema_context.relative_pointer, schema_context.base)
                  .recompose(),
          .schema_resource = schema_resource_id(context.resources, resource),
          .value = value,
          .children = {}};
}

// Instantiate a value-oriented step
inline auto make(const InstructionIndex type, const Context &context,
                 const SchemaContext &schema_context,
                 const DynamicContext &dynamic_context, const Value &value)
    -> Instruction {
  return make_with_resource(type, context, schema_context, dynamic_context,
                            value, schema_context.base.recompose());
}

// Instantiate an applicator step
inline auto make(const InstructionIndex type, const Context &context,
                 const SchemaContext &schema_context,
                 const DynamicContext &dynamic_context, Value &&value,
                 Instructions &&children) -> Instruction {
  const auto schema_location{
      dynamic_context.keyword.empty()
          ? to_pointer(dynamic_context.base_schema_location)
          : to_pointer(dynamic_context.base_schema_location)
                .concat({dynamic_context.keyword})};
  return {.type = type,
          .relative_schema_location = schema_location,
          .relative_instance_location =
              to_pointer(dynamic_context.base_instance_location),
          .keyword_location =
              to_uri(schema_context.relative_pointer, schema_context.base)
                  .recompose(),
          .schema_resource = schema_resource_id(
              context.resources, schema_context.base.recompose()),
          .value = std::move(value),
          .children = std::move(children)};
}

inline auto unroll(const Instruction &step,
                   const sourcemeta::core::WeakPointer &base_instance_location =
                       sourcemeta::core::empty_weak_pointer) -> Instruction {
  return {.type = step.type,
          .relative_schema_location = step.relative_schema_location,
          .relative_instance_location =
              to_pointer(base_instance_location)
                  .concat(step.relative_instance_location),
          .keyword_location = step.keyword_location,
          .schema_resource = step.schema_resource,
          .value = step.value,
          .children = {}};
}

inline auto rephrase(const InstructionIndex type, const Instruction &step)
    -> Instruction {
  return {.type = type,
          .relative_schema_location = step.relative_schema_location,
          .relative_instance_location = step.relative_instance_location,
          .keyword_location = step.keyword_location,
          .schema_resource = step.schema_resource,
          .value = step.value,
          .children = {}};
}

inline auto
unsigned_integer_property(const sourcemeta::core::JSON &document,
                          const sourcemeta::core::JSON::String &property)
    -> std::optional<std::size_t> {
  if (document.defines(property) && document.at(property).is_integer()) {
    const auto value{document.at(property).to_integer()};
    assert(value >= 0);
    return static_cast<std::size_t>(value);
  }

  return std::nullopt;
}

inline auto
unsigned_integer_property(const sourcemeta::core::JSON &document,
                          const sourcemeta::core::JSON::String &property,
                          const std::size_t otherwise) -> std::size_t {
  return unsigned_integer_property(document, property).value_or(otherwise);
}

inline auto static_frame_entry(const Context &context,
                               const SchemaContext &schema_context)
    -> const sourcemeta::core::SchemaFrame::Location & {
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  const auto type{sourcemeta::core::SchemaReferenceType::Static};
  assert(context.frame.locations().contains({type, current}));
  return context.frame.locations().at({type, current});
}

inline auto walk_subschemas(const Context &context,
                            const SchemaContext &schema_context,
                            const DynamicContext &dynamic_context) -> auto {
  const auto &entry{static_frame_entry(context, schema_context)};
  return sourcemeta::core::SchemaIterator{
      schema_context.schema.at(dynamic_context.keyword), context.walker,
      context.resolver, entry.dialect};
}

// TODO: Get rid of this given the new Core regex optimisations
inline auto pattern_as_prefix(const std::string &pattern)
    -> std::optional<std::string> {
  static const std::regex starts_with_regex{R"(^\^([a-zA-Z0-9-_/]+)$)"};
  std::smatch matches;
  if (std::regex_match(pattern, matches, starts_with_regex)) {
    return matches[1].str();
  } else {
    return std::nullopt;
  }
}

inline auto find_adjacent(const Context &context,
                          const SchemaContext &schema_context,
                          const std::set<std::string> &vocabularies,
                          const std::string &keyword,
                          const sourcemeta::core::JSON::Type type) -> auto {
  std::vector<std::string> possible_keyword_uris;
  possible_keyword_uris.push_back(
      to_uri(schema_context.relative_pointer.initial().concat(
                 make_weak_pointer(keyword)),
             schema_context.base)
          .recompose());

  // TODO: Do something similar with `allOf`

  // Attempt to statically follow references
  static const std::string ref_keyword{"$ref"};
  if (schema_context.schema.defines("$ref")) {
    const auto reference_type{sourcemeta::core::SchemaReferenceType::Static};
    const auto destination_uri{
        to_uri(schema_context.relative_pointer.initial().concat(
                   make_weak_pointer(ref_keyword)),
               schema_context.base)
            .recompose()};
    assert(
        context.frame.locations().contains({reference_type, destination_uri}));
    const auto &destination{
        context.frame.locations().at({reference_type, destination_uri})};
    assert(context.frame.references().contains(
        {reference_type, destination.pointer}));
    const auto &reference{
        context.frame.references().at({reference_type, destination.pointer})};
    const auto keyword_uri{
        sourcemeta::core::to_uri(
            sourcemeta::core::to_pointer(
                std::string{reference.fragment.value_or("")})
                .concat({keyword}))
            .resolve_from(sourcemeta::core::URI{reference.base})};

    // TODO: When this logic is used by
    // `unevaluatedProperties`/`unevaluatedItems`, how can we let the
    // applicators we detect here know that they have already been taken into
    // consideration and thus do not have to track evaluation?
    possible_keyword_uris.push_back(keyword_uri.recompose());
  }

  std::vector<std::reference_wrapper<const sourcemeta::core::JSON>> result;

  for (const auto &possible_keyword_uri : possible_keyword_uris) {
    if (!context.frame.locations().contains(
            {sourcemeta::core::SchemaReferenceType::Static,
             possible_keyword_uri})) {
      continue;
    }

    const auto &frame_entry{context.frame.locations().at(
        {sourcemeta::core::SchemaReferenceType::Static, possible_keyword_uri})};
    const auto &subschema{
        sourcemeta::core::get(context.root, frame_entry.pointer)};
    const auto &subschema_vocabularies{sourcemeta::core::vocabularies(
        subschema, context.resolver, frame_entry.dialect)};

    if (std::ranges::any_of(vocabularies,
                            [&subschema_vocabularies](const auto &vocabulary) {
                              return subschema_vocabularies.contains(
                                  vocabulary);
                            }) &&
        subschema.type() == type) {
      result.emplace_back(subschema);
    }
  }

  return result;
}

inline auto recursive_template_size(const Instructions &steps) -> std::size_t {
  std::size_t result{steps.size()};
  for (const auto &variant : steps) {
    result += recursive_template_size(variant.children);
  }

  return result;
}

inline auto make_property(const ValueString &property) -> ValueProperty {
  static const sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  return {property, hasher(property)};
}

inline auto requires_evaluation(const Context &context,
                                const sourcemeta::core::WeakPointer &pointer)
    -> bool {
  for (const auto &unevaluated : context.unevaluated) {
    if (unevaluated.second.unresolved ||
        unevaluated.second.dynamic_dependencies.contains(pointer)) {
      return true;
    }

    for (const auto &dependency : unevaluated.second.dynamic_dependencies) {
      if (dependency.starts_with(pointer)) {
        return true;
      }
    }
  }

  return false;
}

inline auto requires_evaluation(const Context &context,
                                const SchemaContext &schema_context) -> bool {
  const auto &entry{static_frame_entry(context, schema_context)};
  return requires_evaluation(context, entry.pointer);
}

// TODO: Elevate to Core and test

inline auto
is_circular(const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::WeakPointer &reference_origin,
            const sourcemeta::core::SchemaFrame::ReferencesEntry &reference,
            std::unordered_set<std::string> &visited) -> bool {
  if (visited.contains(reference.destination)) {
    return false;
  }
  visited.insert(reference.destination);

  const auto destination_location{frame.traverse(reference.destination)};
  if (!destination_location.has_value()) {
    return false;
  }

  const auto &destination_pointer{destination_location->get().pointer};
  if (reference_origin.starts_with(destination_pointer) ||
      destination_pointer.starts_with(reference_origin)) {
    return true;
  }

  for (const auto &ref_entry : frame.references()) {
    if (ref_entry.first.first ==
            sourcemeta::core::SchemaReferenceType::Static &&
        ref_entry.first.second.starts_with(destination_pointer)) {
      if (is_circular(frame, reference_origin, ref_entry.second, visited)) {
        return true;
      }
    }
  }

  return false;
}

} // namespace sourcemeta::blaze

#endif
