#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2019_09_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2019_09_H_

#include <sourcemeta/blaze/compiler.h>

#include <utility> // std::to_underlying

#include "compile_helpers.h"
#include "default_compiler_draft4.h"
#include "upstream.h"

namespace sourcemeta::blaze {

auto compiler_2019_09_applicator_dependentschemas(const KeywordContext &context,
                                                  const Instructions &)
    -> Instructions {
  if (!context.value().is_object()) {
    throw_keyword_error(context, EXPECTED_OBJECT);
  }

  if (!std::ranges::all_of(
          context.value().as_object(),
          [allow_boolean = booleans_are_schemas(context.vocabularies())](
              const auto &entry) -> bool {
            return is_schema(entry.second, allow_boolean);
          })) {
    throw_keyword_error(context, EXPECTED_SCHEMA_OBJECT);
  }

  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "object") {
    return {};
  }

  Instructions children;

  // To guarantee order
  std::vector<std::string> dependents;
  for (const auto &entry : context.value().as_object()) {
    dependents.push_back(entry.first);
  }
  std::ranges::sort(dependents);

  for (const auto &dependent : dependents) {
    const auto &dependency{context.value().at(dependent)};
    if (!(dependency.is_object() || dependency.is_boolean())) {
      continue;
    }

    if (!dependency.is_boolean() || !dependency.to_boolean()) {
      children.push_back(
          make(InstructionIndex::LogicalWhenDefines, context.relative(),
               make_property(dependent),
               compile(context.relative(), make_weak_pointer(dependent))));
    }
  }

  if (children.empty()) {
    return {};
  }

  // TODO: Is this wrapper really necessary?
  return {make(InstructionIndex::LogicalWhenType, context,
               sourcemeta::core::JSON::Type::Object, std::move(children))};
}

auto compiler_2019_09_validation_dependentrequired(
    const KeywordContext &context, const Instructions &) -> Instructions {
  if (!context.value().is_object()) {
    throw_keyword_error(context, EXPECTED_OBJECT);
  }

  if (!std::ranges::all_of(
          context.value().as_object(), [](const auto &entry) -> bool {
            return sourcemeta::core::schema_is_property_name_array(
                entry.second);
          })) {
    throw_keyword_error(context, EXPECTED_DEPENDENCIES);
  }

  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "object") {
    return {};
  }

  ValueStringMap dependencies;
  for (const auto &entry : context.value().as_object()) {

    std::vector<sourcemeta::core::JSON::String> properties;
    for (const auto &property : entry.second.as_array()) {
      if (!property.is_string()) {
        continue;
      }

      properties.push_back(property.to_string());
    }

    if (!properties.empty()) {
      dependencies.emplace(entry.first, properties);
    }
  }

  if (dependencies.empty()) {
    return {};
  }

  return {make(InstructionIndex::AssertionPropertyDependencies, context,
               std::move(dependencies))};
}

auto compiler_2019_09_core_annotation(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  return {make(InstructionIndex::AnnotationEmit, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_2019_09_applicator_contains_with_options(
    const KeywordContext &context, const Instructions &, const bool annotate,
    const bool track_evaluation) -> Instructions {
  if (context.local.is_property_name) {
    return {};
  }

  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "array") {
    return {};
  }

  // These bounds accept any integral number, including one spelled with a
  // fractional or exponent part like `1.0` or `1e0`, as this dialect reads
  // every number with a zero fractional part as an integer. A bound too large
  // to hold is left out, as the general check below cannot carry it
  std::size_t minimum{1};
  bool minimum_is_unreachable{false};
  if (context.schema().defines("minContains") &&
      context.schema().at("minContains").is_integral() &&
      context.schema().at("minContains").is_positive()) {
    if (bound_is_representable(context.schema().at("minContains"))) {
      minimum = static_cast<std::size_t>(
          context.schema().at("minContains").as_integer());
    } else {
      minimum_is_unreachable = true;
    }
  }

  std::optional<std::size_t> maximum;
  if (context.schema().defines("maxContains") &&
      context.schema().at("maxContains").is_integral() &&
      context.schema().at("maxContains").is_positive() &&
      bound_is_representable(context.schema().at("maxContains"))) {
    maximum = static_cast<std::size_t>(
        context.schema().at("maxContains").as_integer());
  }

  // A lower bound too large to hold is one no array has enough items to meet,
  // so every array fails and non-arrays are left alone, which is what this
  // keyword asks for. Carrying such a bound into the general check below would
  // instead report a count the schema never stated
  if (minimum_is_unreachable) {
    ValueTypes types;
    types.set(std::to_underlying(sourcemeta::core::JSON::Type::Array));
    return {make(InstructionIndex::AssertionNotTypeStrictAny, context, types)};
  }

  // An unsatisfiable range is deliberately left for the general check below,
  // which rejects every array yet leaves non-arrays alone. An unconditional
  // failure here would instead reject non-arrays too, but `contains` does not
  // apply to them

  if (minimum == 0 && !maximum.has_value() && !track_evaluation) {
    return {};
  }

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (annotate && annotations_enabled(context)) {
    children.push_back(make(InstructionIndex::AnnotationBasenameToParent,
                            context.relative(), ValueNone{}));

    // TODO: If after emitting the above annotation, the number of annotations
    // for the current schema location + instance location is equal to the
    // array size (which means we annotated all of the items), then emit
    // an annotation "true"
  }

  if (track_evaluation) {
    children.push_back(make(InstructionIndex::ControlEvaluate,
                            context.relative(), ValuePointer{}));
  }

  // A trivial subschema matches every element, so the number of matches
  // equals the array size and the constraint reduces to a size bound. The
  // plain "at least one match" case has a cheap single-instruction form, but
  // any higher minimum or any maximum needs the general range check, which
  // handles an empty subschema correctly on its own
  if (children.empty() && minimum == 1 && !maximum.has_value()) {
    return {make(InstructionIndex::AssertionArraySizeGreater, context,
                 ValueUnsignedInteger{0})};
  }

  return {make(InstructionIndex::LoopContains, context,
               ValueRange{minimum, maximum, annotate || track_evaluation},
               std::move(children))};
}

auto compiler_2019_09_applicator_contains(const KeywordContext &context,
                                          const Instructions &current)
    -> Instructions {
  return compiler_2019_09_applicator_contains_with_options(
      context, current, context.global.collects_annotations, false);
}

auto compiler_2019_09_applicator_additionalproperties(
    const KeywordContext &context, const Instructions &)

    -> Instructions {
  return compiler_draft3_applicator_additionalproperties_with_options(
      context, context.global.collects_annotations,
      requires_evaluation(context));
}

auto compiler_2019_09_applicator_items(const KeywordContext &context,
                                       const Instructions &) -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  if (context.value().is_array()) {
    return compiler_draft3_applicator_items_with_options(
        context, context.global.collects_annotations, track);
  }

  return compiler_draft3_applicator_items_with_options(
      context, context.global.collects_annotations,
      track && !context.schema().defines("unevaluatedItems"));
}

auto compiler_2019_09_applicator_additionalitems(const KeywordContext &context,
                                                 const Instructions &)
    -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  return compiler_draft3_applicator_additionalitems_with_options(
      context, context.global.collects_annotations,
      track && !context.schema().defines("unevaluatedItems"));
}

auto compiler_2019_09_applicator_unevaluateditems(const KeywordContext &context,
                                                  const Instructions &)
    -> Instructions {
  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "array") {
    return {};
  }

  const auto current_uri{
      to_uri(context.local.relative_pointer, context.base()).recompose()};
  assert(context.global.unevaluated.contains(current_uri));
  const auto &dependencies{context.global.unevaluated.at(current_uri)};

  for (const auto &dependency : dependencies.static_dependencies) {
    assert(!dependency.empty());
    assert(dependency.back().is_property());
    const auto &keyword{dependency.back().to_property()};
    const auto &subschema{
        sourcemeta::core::get(context.global.root, dependency)};
    // NOLINTBEGIN(bugprone-branch-clone)
    if (keyword == "items" &&
        (subschema.is_object() || subschema.is_boolean())) {
      return {};
    }
    if (keyword == "additionalItems" || keyword == "unevaluatedItems") {
      return {};
    }
    // NOLINTEND(bugprone-branch-clone)
  }

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (annotations_enabled(context)) {
    children.push_back(make(InstructionIndex::AnnotationToParent,
                            context.relative(), sourcemeta::core::JSON{true}));
  }

  if (children.empty()) {
    if (dependencies.dynamic_dependencies.empty() && !dependencies.unresolved &&
        !requires_evaluation(context)) {
      return {};
    }

    // An unconditional marker would record this instance as evaluated whatever
    // its type, so against a non-array it would wrongly suppress a sibling
    // `unevaluatedProperties`. An array-guarded marker only records arrays
    return {make(InstructionIndex::LoopItemsUnevaluated, context, ValueNone{},
                 Instructions{})};
  }

  // TODO: Attempt to short-circuit evaluation tracking by looking at sibling
  // and adjacent keywords like we do for `unevaluatedProperties`

  return {make(InstructionIndex::LoopItemsUnevaluated, context, ValueNone{},
               std::move(children))};
}

auto compiler_2019_09_applicator_unevaluatedproperties(
    const KeywordContext &context, const Instructions &) -> Instructions {
  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "object") {
    return {};
  }

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (annotations_enabled(context)) {
    children.push_back(make(InstructionIndex::AnnotationBasenameToParent,
                            context.relative(), ValueNone{}));
  }

  ValueStringSet filter_strings;
  ValueStrings filter_prefixes;
  std::vector<ValueRegex> filter_regexes;

  const auto current_uri{
      to_uri(context.local.relative_pointer, context.base()).recompose()};
  assert(context.global.unevaluated.contains(current_uri));
  const auto &dependencies{context.global.unevaluated.at(current_uri)};

  for (const auto &dependency : dependencies.static_dependencies) {
    assert(!dependency.empty());
    assert(dependency.back().is_property());
    const auto &keyword{dependency.back().to_property()};
    const auto &subschema{
        sourcemeta::core::get(context.global.root, dependency)};
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
            static const std::string PATTERN_PROPERTIES_KEYWORD{
                "patternProperties"};
            filter_regexes.push_back(
                {.first = parse_regex(
                     property.first, context,
                     context.local.relative_pointer.initial().concat(
                         make_weak_pointer(PATTERN_PROPERTIES_KEYWORD))),
                 .second = property.first});
          }
        }
      }
    } else if (keyword == "additionalProperties" ||
               keyword == "unevaluatedProperties") {
      return {};
    }
  }

  if (dependencies.dynamic_dependencies.empty() && !dependencies.unresolved &&
      !requires_evaluation(context)) {
    if (children.empty()) {
      return {};
    }
    if (!filter_strings.empty() || !filter_prefixes.empty() ||
        !filter_regexes.empty()) {
      return {make(InstructionIndex::LoopPropertiesExcept, context,
                   ValuePropertyFilter{std::move(filter_strings),
                                       std::move(filter_prefixes),
                                       std::move(filter_regexes)},
                   std::move(children))};
    }
    return {make(InstructionIndex::LoopProperties, context, ValueNone{},
                 std::move(children))};
  }

  if (children.empty()) {
    // An unconditional marker would record this instance as evaluated whatever
    // its type, so against a non-object it would wrongly suppress a sibling
    // `unevaluatedItems`. An object-guarded marker only records objects
    return {make(InstructionIndex::LoopPropertiesEvaluate, context, ValueNone{},
                 Instructions{})};
  }
  if (!filter_strings.empty() || !filter_prefixes.empty() ||
      !filter_regexes.empty()) {
    return {make(InstructionIndex::LoopPropertiesUnevaluatedExcept, context,
                 ValuePropertyFilter{std::move(filter_strings),
                                     std::move(filter_prefixes),
                                     std::move(filter_regexes)},
                 std::move(children))};
  }
  return {make(InstructionIndex::LoopPropertiesUnevaluated, context,
               ValueNone{}, std::move(children))};
}

auto compiler_2019_09_core_recursiveref(const KeywordContext &context,
                                        const Instructions &current)
    -> Instructions {
  const auto entry_pointer{absolute_schema_pointer(context)};
  // In this case, just behave as a normal static reference
  if (!context.global.frame
           .reference(sourcemeta::core::SchemaReferenceType::Dynamic,
                      entry_pointer)
           .has_value()) {
    return compiler_draft3_core_ref(context, current);
  }

  return {make(InstructionIndex::ControlDynamicAnchorJump, context, "")};
}

auto compiler_2019_09_applicator_properties(const KeywordContext &context,
                                            const Instructions &current)
    -> Instructions {
  return compiler_draft3_applicator_properties_with_options(
      context, current, context.global.collects_annotations,
      requires_evaluation(context));
}

auto compiler_2019_09_applicator_patternproperties(
    const KeywordContext &context, const Instructions &) -> Instructions {
  return compiler_draft3_applicator_patternproperties_with_options(
      context, context.global.collects_annotations,
      requires_evaluation(context));
}

auto compiler_2019_09_content_contentencoding(const KeywordContext &context,
                                              const Instructions &)
    -> Instructions {
  if (!annotations_enabled(context)) {
    return {};
  }

  Instructions children{
      make(InstructionIndex::AnnotationEmit, context,
           sourcemeta::core::JSON{context.schema().at(context.keyword())})};

  return {make(InstructionIndex::ControlGroupWhenType, context.relative(),
               ValueType::String, std::move(children))};
}

auto compiler_2019_09_content_contentmediatype(const KeywordContext &context,
                                               const Instructions &)
    -> Instructions {
  if (!annotations_enabled(context)) {
    return {};
  }

  Instructions children{
      make(InstructionIndex::AnnotationEmit, context,
           sourcemeta::core::JSON{context.schema().at(context.keyword())})};

  return {make(InstructionIndex::ControlGroupWhenType, context.relative(),
               ValueType::String, std::move(children))};
}

auto compiler_2019_09_content_contentschema(const KeywordContext &context,
                                            const Instructions &)
    -> Instructions {
  if (!annotations_enabled(context)) {
    return {};
  }

  // The `contentSchema` keyword does nothing without `contentMediaType`
  if (!context.schema().defines("contentMediaType")) {
    return {};
  }

  Instructions children{
      make(InstructionIndex::AnnotationEmit, context,
           sourcemeta::core::JSON{context.schema().at(context.keyword())})};

  return {make(InstructionIndex::ControlGroupWhenType, context.relative(),
               ValueType::String, std::move(children))};
}

} // namespace sourcemeta::blaze
#endif
