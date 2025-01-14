#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_HELPERS_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <algorithm> // std::find
#include <cassert>   // assert
#include <iterator>  // std::distance
#include <regex>     // std::regex, std::regex_match, std::smatch
#include <utility>   // std::declval, std::move
#include <variant>   // std::visit

namespace sourcemeta::blaze {

inline auto relative_dynamic_context() -> DynamicContext {
  return {"", sourcemeta::jsontoolkit::empty_pointer,
          sourcemeta::jsontoolkit::empty_pointer, false};
}

inline auto relative_dynamic_context(const DynamicContext &dynamic_context)
    -> DynamicContext {
  return {"", sourcemeta::jsontoolkit::empty_pointer,
          sourcemeta::jsontoolkit::empty_pointer,
          dynamic_context.property_as_target};
}

inline auto property_relative_dynamic_context() -> DynamicContext {
  return {"", sourcemeta::jsontoolkit::empty_pointer,
          sourcemeta::jsontoolkit::empty_pointer, true};
}

inline auto schema_resource_id(const Context &context,
                               const std::string &resource) -> std::size_t {
  const auto iterator{std::find(
      context.resources.cbegin(), context.resources.cend(),
      sourcemeta::jsontoolkit::URI{resource}.canonicalize().recompose())};
  if (iterator == context.resources.cend()) {
    assert(resource.empty());
    return 0;
  }

  return 1 + static_cast<std::size_t>(
                 std::distance(context.resources.cbegin(), iterator));
}

// Instantiate a value-oriented step
inline auto make(const InstructionIndex type, const Context &context,
                 const SchemaContext &schema_context,
                 const DynamicContext &dynamic_context, const Value &value)
    -> Instruction {
  return {
      type,
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location
          : dynamic_context.base_schema_location.concat(
                {dynamic_context.keyword}),
      dynamic_context.base_instance_location,
      to_uri(schema_context.relative_pointer, schema_context.base).recompose(),
      schema_resource_id(context, schema_context.base.recompose()),
      value,
      {}};
}

// Instantiate an applicator step
inline auto make(const InstructionIndex type, const Context &context,
                 const SchemaContext &schema_context,
                 const DynamicContext &dynamic_context, Value &&value,
                 Instructions &&children) -> Instruction {
  return {
      type,
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location
          : dynamic_context.base_schema_location.concat(
                {dynamic_context.keyword}),
      dynamic_context.base_instance_location,
      to_uri(schema_context.relative_pointer, schema_context.base).recompose(),
      schema_resource_id(context, schema_context.base.recompose()),
      std::move(value),
      std::move(children)};
}

inline auto
unroll(const Instruction &step,
       const sourcemeta::jsontoolkit::Pointer &base_instance_location =
           sourcemeta::jsontoolkit::empty_pointer) -> Instruction {
  return {step.type,
          step.relative_schema_location,
          base_instance_location.concat(step.relative_instance_location),
          step.keyword_location,
          step.schema_resource,
          step.value,
          {}};
}

inline auto rephrase(const InstructionIndex type, const Instruction &step)
    -> Instruction {
  return {type,
          step.relative_schema_location,
          step.relative_instance_location,
          step.keyword_location,
          step.schema_resource,
          step.value,
          {}};
}

inline auto
unsigned_integer_property(const sourcemeta::jsontoolkit::JSON &document,
                          const sourcemeta::jsontoolkit::JSON::String &property)
    -> std::optional<std::size_t> {
  if (document.defines(property) && document.at(property).is_integer()) {
    const auto value{document.at(property).to_integer()};
    assert(value >= 0);
    return static_cast<std::size_t>(value);
  }

  return std::nullopt;
}

inline auto
unsigned_integer_property(const sourcemeta::jsontoolkit::JSON &document,
                          const sourcemeta::jsontoolkit::JSON::String &property,
                          const std::size_t otherwise) -> std::size_t {
  return unsigned_integer_property(document, property).value_or(otherwise);
}

inline auto static_frame_entry(const Context &context,
                               const SchemaContext &schema_context)
    -> const sourcemeta::jsontoolkit::Frame::LocationsEntry & {
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  const auto type{sourcemeta::jsontoolkit::ReferenceType::Static};
  assert(context.frame.locations().contains({type, current}));
  return context.frame.locations().at({type, current});
}

inline auto walk_subschemas(const Context &context,
                            const SchemaContext &schema_context,
                            const DynamicContext &dynamic_context) -> auto {
  const auto &entry{static_frame_entry(context, schema_context)};
  return sourcemeta::jsontoolkit::SchemaIterator{
      schema_context.schema.at(dynamic_context.keyword), context.walker,
      context.resolver, entry.dialect};
}

// TODO: Get rid of this given the new JSON Toolkit regex optimisations
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
                          const sourcemeta::jsontoolkit::JSON::Type type)
    -> auto {
  std::vector<std::string> possible_keyword_uris;
  possible_keyword_uris.push_back(
      to_uri(schema_context.relative_pointer.initial().concat({keyword}),
             schema_context.base)
          .recompose());

  // TODO: Do something similar with `allOf`

  // Attempt to statically follow references
  if (schema_context.schema.defines("$ref")) {
    const auto reference_type{sourcemeta::jsontoolkit::ReferenceType::Static};
    const auto destination_uri{
        to_uri(schema_context.relative_pointer.initial().concat({"$ref"}),
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
        sourcemeta::jsontoolkit::to_uri(
            sourcemeta::jsontoolkit::to_pointer(reference.fragment.value_or(""))
                .concat({keyword}))
            .try_resolve_from(reference.base.value_or(""))};

    // TODO: When this logic is used by
    // `unevaluatedProperties`/`unevaluatedItems`, how can we let the
    // applicators we detect here know that they have already been taken into
    // consideration and thus do not have to track evaluation?
    possible_keyword_uris.push_back(keyword_uri.recompose());
  }

  std::vector<std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>>
      result;

  for (const auto &possible_keyword_uri : possible_keyword_uris) {
    if (!context.frame.locations().contains(
            {sourcemeta::jsontoolkit::ReferenceType::Static,
             possible_keyword_uri})) {
      continue;
    }

    const auto &frame_entry{context.frame.locations().at(
        {sourcemeta::jsontoolkit::ReferenceType::Static,
         possible_keyword_uri})};
    const auto &subschema{
        sourcemeta::jsontoolkit::get(context.root, frame_entry.pointer)};
    const auto &subschema_vocabularies{sourcemeta::jsontoolkit::vocabularies(
        subschema, context.resolver, frame_entry.dialect)};

    if (std::any_of(vocabularies.cbegin(), vocabularies.cend(),
                    [&subschema_vocabularies](const auto &vocabulary) {
                      return subschema_vocabularies.contains(vocabulary);
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
  static const sourcemeta::jsontoolkit::Hash hasher;
  return {property, hasher(property)};
}

inline auto requires_evaluation(const Context &context,
                                const SchemaContext &schema_context) -> bool {
  const auto &entry{static_frame_entry(context, schema_context)};
  for (const auto &unevaluated : context.unevaluated) {
    if (unevaluated.second.unresolved ||
        unevaluated.second.dynamic_dependencies.contains(entry.pointer)) {
      return true;
    }

    for (const auto &dependency : unevaluated.second.dynamic_dependencies) {
      if (dependency.starts_with(entry.pointer)) {
        return true;
      }
    }
  }

  return false;
}

} // namespace sourcemeta::blaze

#endif
