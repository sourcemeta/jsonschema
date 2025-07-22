#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2019_09_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2019_09_H_

#include <sourcemeta/blaze/compiler.h>

#include "compile_helpers.h"
#include "default_compiler_draft4.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_2019_09_applicator_dependentschemas(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  assert(schema_context.schema.at(dynamic_context.keyword).is_object());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Instructions children;

  // To guarantee order
  std::vector<std::string> dependents;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    dependents.push_back(entry.first);
  }
  std::sort(dependents.begin(), dependents.end());

  for (const auto &dependent : dependents) {
    const auto &dependency{
        schema_context.schema.at(dynamic_context.keyword).at(dependent)};
    if (!is_schema(dependency)) {
      continue;
    }

    if (!dependency.is_boolean() || !dependency.to_boolean()) {
      children.push_back(
          make(sourcemeta::blaze::InstructionIndex::LogicalWhenDefines, context,
               schema_context, relative_dynamic_context(dynamic_context),
               make_property(dependent),
               compile(context, schema_context,
                       relative_dynamic_context(dynamic_context), {dependent},
                       sourcemeta::core::empty_pointer)));
    }
  }

  // TODO: Is this wrapper really necessary?
  return {make(sourcemeta::blaze::InstructionIndex::LogicalWhenType, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON::Type::Object, std::move(children))};
}

auto compiler_2019_09_validation_dependentrequired(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_object()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  ValueStringMap dependencies;
  for (const auto &entry :
       schema_context.schema.at(dynamic_context.keyword).as_object()) {
    if (!entry.second.is_array()) {
      continue;
    }

    std::vector<sourcemeta::core::JSON::String> properties;
    for (const auto &property : entry.second.as_array()) {
      assert(property.is_string());
      properties.push_back(property.to_string());
    }

    if (!properties.empty()) {
      dependencies.emplace(entry.first, std::move(properties));
    }
  }

  if (dependencies.empty()) {
    return {};
  }

  return {
      make(sourcemeta::blaze::InstructionIndex::AssertionPropertyDependencies,
           context, schema_context, dynamic_context, std::move(dependencies))};
}

auto compiler_2019_09_core_annotation(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  return {make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_2019_09_applicator_contains_with_options(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &,
    const bool annotate, const bool track_evaluation) -> Instructions {
  if (schema_context.is_property_name) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  std::size_t minimum{1};
  if (schema_context.schema.defines("minContains")) {
    if (schema_context.schema.at("minContains").is_integer() &&
        schema_context.schema.at("minContains").is_positive()) {
      minimum = static_cast<std::size_t>(
          schema_context.schema.at("minContains").to_integer());
    } else if (schema_context.schema.at("minContains").is_real() &&
               schema_context.schema.at("minContains").is_positive()) {
      minimum = static_cast<std::size_t>(
          schema_context.schema.at("minContains").as_integer());
    }
  }

  std::optional<std::size_t> maximum;
  if (schema_context.schema.defines("maxContains")) {
    if (schema_context.schema.at("maxContains").is_integer() &&
        schema_context.schema.at("maxContains").is_positive()) {
      maximum = schema_context.schema.at("maxContains").to_integer();
    } else if (schema_context.schema.at("maxContains").is_real() &&
               schema_context.schema.at("maxContains").is_positive()) {
      maximum = schema_context.schema.at("maxContains").as_integer();
    }
  }

  if (maximum.has_value() && minimum > maximum.value()) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

  if (minimum == 0 && !maximum.has_value() && !track_evaluation) {
    return {};
  }

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  if (annotate) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
             context, schema_context, relative_dynamic_context(dynamic_context),
             ValueNone{}));

    // TODO: If after emitting the above annotation, the number of annotations
    // for the current schema location + instance location is equal to the
    // array size (which means we annotated all of the items), then emit
    // an annotation "true"
  }

  if (track_evaluation) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::ControlEvaluate, context,
             schema_context, relative_dynamic_context(dynamic_context),
             ValuePointer{}));
  }

  if (children.empty()) {
    // We still need to check the instance is not empty
    return {make(sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater,
                 context, schema_context, dynamic_context,
                 ValueUnsignedInteger{0})};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LoopContains, context,
               schema_context, dynamic_context,
               ValueRange{minimum, maximum, annotate || track_evaluation},
               std::move(children))};
}

auto compiler_2019_09_applicator_contains(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &current)
    -> Instructions {
  return compiler_2019_09_applicator_contains_with_options(
      context, schema_context, dynamic_context, current, false, false);
}

auto compiler_2019_09_applicator_additionalproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)

    -> Instructions {
  return compiler_draft4_applicator_additionalproperties_with_options(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive,
      requires_evaluation(context, schema_context));
}

auto compiler_2019_09_applicator_items(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context,
                                       const Instructions &) -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{
      std::any_of(context.unevaluated.cbegin(), context.unevaluated.cend(),
                  [](const auto &dependency) {
                    return dependency.first.ends_with("unevaluatedItems");
                  })};

  if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return compiler_draft4_applicator_items_with_options(
        context, schema_context, dynamic_context,
        context.mode == Mode::Exhaustive, track);
  }

  return compiler_draft4_applicator_items_with_options(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive,
      track && !schema_context.schema.defines("unevaluatedItems"));
}

auto compiler_2019_09_applicator_additionalitems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{
      std::any_of(context.unevaluated.cbegin(), context.unevaluated.cend(),
                  [](const auto &dependency) {
                    return dependency.first.ends_with("unevaluatedItems");
                  })};

  return compiler_draft4_applicator_additionalitems_with_options(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive,
      track && !schema_context.schema.defines("unevaluatedItems"));
}

auto compiler_2019_09_applicator_unevaluateditems(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  const auto current_uri{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.unevaluated.contains(current_uri));
  const auto &dependencies{context.unevaluated.at(current_uri)};

  for (const auto &dependency : dependencies.static_dependencies) {
    assert(!dependency.empty());
    assert(dependency.back().is_property());
    const auto &keyword{dependency.back().to_property()};
    const auto &subschema{sourcemeta::core::get(context.root, dependency)};
    if (keyword == "items" && sourcemeta::core::is_schema(subschema)) {
      return {};
    } else if (keyword == "additionalItems" || keyword == "unevaluatedItems") {
      return {};
    }
  }

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  if (context.mode == Mode::Exhaustive) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::AnnotationToParent, context,
             schema_context, relative_dynamic_context(dynamic_context),
             sourcemeta::core::JSON{true}));
  }

  if (children.empty()) {
    if (dependencies.dynamic_dependencies.empty() && !dependencies.unresolved &&
        !requires_evaluation(context, schema_context)) {
      return {};
    }

    return {make(sourcemeta::blaze::InstructionIndex::Evaluate, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

  // TODO: Attempt to short-circuit evaluation tracking by looking at sibling
  // and adjacent keywords like we do for `unevaluatedProperties`

  return {make(sourcemeta::blaze::InstructionIndex::LoopItemsUnevaluated,
               context, schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

auto compiler_2019_09_applicator_unevaluatedproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  if (context.mode == Mode::Exhaustive) {
    children.push_back(
        make(sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent,
             context, schema_context, relative_dynamic_context(dynamic_context),
             ValueNone{}));
  }

  ValueStringSet filter_strings;
  ValueStrings filter_prefixes;
  std::vector<ValueRegex> filter_regexes;

  const auto current_uri{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.unevaluated.contains(current_uri));
  const auto &dependencies{context.unevaluated.at(current_uri)};

  for (const auto &dependency : dependencies.static_dependencies) {
    assert(!dependency.empty());
    assert(dependency.back().is_property());
    const auto &keyword{dependency.back().to_property()};
    const auto &subschema{sourcemeta::core::get(context.root, dependency)};
    if (keyword == "properties") {
      if (subschema.is_object()) {
        for (const auto &property : subschema.as_object()) {
          filter_strings.insert(property.first);
        }
      }
    } else if (keyword == "patternProperties") {
      if (subschema.is_object()) {
        for (const auto &property : subschema.as_object()) {
          const auto maybe_prefix{pattern_as_prefix(property.first)};
          if (maybe_prefix.has_value()) {
            filter_prefixes.push_back(maybe_prefix.value());
          } else {
            filter_regexes.push_back(
                {parse_regex(property.first, schema_context.base,
                             schema_context.relative_pointer.initial().concat(
                                 {"patternProperties"})),
                 property.first});
          }
        }
      }
    } else if (keyword == "additionalProperties" ||
               keyword == "unevaluatedProperties") {
      return {};
    }
  }

  if (dependencies.dynamic_dependencies.empty() && !dependencies.unresolved &&
      !requires_evaluation(context, schema_context)) {
    if (children.empty()) {
      return {};
    } else if (!filter_strings.empty() || !filter_prefixes.empty() ||
               !filter_regexes.empty()) {
      return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesExcept,
                   context, schema_context, dynamic_context,
                   ValuePropertyFilter{std::move(filter_strings),
                                       std::move(filter_prefixes),
                                       std::move(filter_regexes)},
                   std::move(children))};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::LoopProperties, context,
                   schema_context, dynamic_context, ValueNone{},
                   std::move(children))};
    }
  }

  if (children.empty()) {
    return {make(sourcemeta::blaze::InstructionIndex::Evaluate, context,
                 schema_context, dynamic_context, ValueNone{})};
  } else if (!filter_strings.empty() || !filter_prefixes.empty() ||
             !filter_regexes.empty()) {
    return {make(
        sourcemeta::blaze::InstructionIndex::LoopPropertiesUnevaluatedExcept,
        context, schema_context, dynamic_context,
        ValuePropertyFilter{std::move(filter_strings),
                            std::move(filter_prefixes),
                            std::move(filter_regexes)},
        std::move(children))};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LoopPropertiesUnevaluated,
                 context, schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_2019_09_core_recursiveref(const Context &context,
                                        const SchemaContext &schema_context,
                                        const DynamicContext &dynamic_context,
                                        const Instructions &current)
    -> Instructions {
  const auto &entry{static_frame_entry(context, schema_context)};
  // In this case, just behave as a normal static reference
  if (!context.frame.references().contains(
          {sourcemeta::core::SchemaReferenceType::Dynamic, entry.pointer})) {
    return compiler_draft4_core_ref(context, schema_context, dynamic_context,
                                    current);
  }

  return {make(sourcemeta::blaze::InstructionIndex::ControlDynamicAnchorJump,
               context, schema_context, dynamic_context, "")};
}

auto compiler_2019_09_applicator_properties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &current)
    -> Instructions {
  return compiler_draft4_applicator_properties_with_options(
      context, schema_context, dynamic_context, current,
      context.mode == Mode::Exhaustive,
      requires_evaluation(context, schema_context));
}

auto compiler_2019_09_applicator_patternproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compiler_draft4_applicator_patternproperties_with_options(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive,
      requires_evaluation(context, schema_context));
}

auto compiler_2019_09_content_contentencoding(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (context.mode == Mode::FastValidation) {
    return {};
  }

  Instructions children{
      make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
           schema_context, dynamic_context,
           sourcemeta::core::JSON{
               schema_context.schema.at(dynamic_context.keyword)})};

  return {make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
               context, schema_context,
               relative_dynamic_context(dynamic_context), ValueType::String,
               std::move(children))};
}

auto compiler_2019_09_content_contentmediatype(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (context.mode == Mode::FastValidation) {
    return {};
  }

  Instructions children{
      make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
           schema_context, dynamic_context,
           sourcemeta::core::JSON{
               schema_context.schema.at(dynamic_context.keyword)})};

  return {make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
               context, schema_context,
               relative_dynamic_context(dynamic_context), ValueType::String,
               std::move(children))};
}

auto compiler_2019_09_content_contentschema(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (context.mode == Mode::FastValidation) {
    return {};
  }

  // The `contentSchema` keyword does nothing without `contentMediaType`
  if (!schema_context.schema.defines("contentMediaType")) {
    return {};
  }

  Instructions children{
      make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
           schema_context, dynamic_context,
           sourcemeta::core::JSON{
               schema_context.schema.at(dynamic_context.keyword)})};

  return {make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
               context, schema_context,
               relative_dynamic_context(dynamic_context), ValueType::String,
               std::move(children))};
}

auto compiler_2019_09_format_format(const Context &context,
                                    const SchemaContext &schema_context,
                                    const DynamicContext &dynamic_context,
                                    const Instructions &) -> Instructions {
  if (context.mode == Mode::FastValidation) {
    return {};
  }

  Instructions children{
      make(sourcemeta::blaze::InstructionIndex::AnnotationEmit, context,
           schema_context, dynamic_context,
           sourcemeta::core::JSON{
               schema_context.schema.at(dynamic_context.keyword)})};

  return {make(sourcemeta::blaze::InstructionIndex::ControlGroupWhenType,
               context, schema_context,
               relative_dynamic_context(dynamic_context), ValueType::String,
               std::move(children))};
}

} // namespace internal
#endif
