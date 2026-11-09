#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2020_12_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2020_12_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/uri.h>

#include "compile_helpers.h"
#include "default_compiler_draft4.h"

namespace sourcemeta::blaze {

auto compiler_2020_12_applicator_prefixitems(const KeywordContext &context,
                                             const Instructions &)
    -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  return compiler_draft3_applicator_items_array(
      context, context.global.collects_annotations, track);
}

auto compiler_2020_12_applicator_items(const KeywordContext &context,
                                       const Instructions &) -> Instructions {
  const auto cursor{(context.schema().defines("prefixItems") &&
                     context.schema().at("prefixItems").is_array())
                        ? context.schema().at("prefixItems").size()
                        : 0};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  return compiler_draft3_applicator_additionalitems_from_cursor(
      context, cursor, context.global.collects_annotations,
      track && !context.schema().defines("unevaluatedItems"));
}

auto compiler_2020_12_applicator_contains(const KeywordContext &context,
                                          const Instructions &current)
    -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  return compiler_2019_09_applicator_contains_with_options(
      context, current, context.global.collects_annotations, track);
}

auto compiler_2020_12_core_dynamicref(const KeywordContext &context,
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

  assert(context.value().is_string());
  sourcemeta::core::URI reference{context.value().to_string()};
  reference.resolve_from(context.base());
  reference.canonicalize();
  // We handle the non-anchor variant by not treating it as a dynamic reference
  assert(reference.fragment().has_value());

  // TODO: Here we can potentially optimize `$dynamicRef` as a static reference
  // if we determine (by traversing the frame) that the given dynamic anchor
  // is only defined once. That means there is only one schema resource, and
  // the jump and be always statically determined

  // Note we don't need to even care about the static part of the dynamic
  // reference (if any), as even if we jump first there, we will still
  // look for the oldest dynamic anchor in the schema resource chain.

  if (reference.is_fragment_only()) {
    return {make(InstructionIndex::ControlDynamicAnchorJump, context,
                 std::string{reference.fragment().value()})};
  }
  const auto base_resource{reference.recompose_without_fragment()};
  assert(base_resource.has_value());

  // If the dynamic reference has a static component, we need to make sure we
  // append such static part as a resource before we begin the lookup
  return {make_with_resource(
      InstructionIndex::ControlDynamicAnchorJump, context,
      // TODO: The amount of possible anchors is known at compile time.
      // We could convert it into integers like we do for resources
      std::string{reference.fragment().value()}, base_resource.value())};
}

} // namespace sourcemeta::blaze
#endif
