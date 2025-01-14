#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2020_12_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_2020_12_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "compile_helpers.h"
#include "default_compiler_draft4.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_2020_12_applicator_prefixitems(
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

  return compiler_draft4_applicator_items_array(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive, track);
}

auto compiler_2020_12_applicator_items(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context,
                                       const Instructions &) -> Instructions {
  const auto cursor{(schema_context.schema.defines("prefixItems") &&
                     schema_context.schema.at("prefixItems").is_array())
                        ? schema_context.schema.at("prefixItems").size()
                        : 0};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{
      std::any_of(context.unevaluated.cbegin(), context.unevaluated.cend(),
                  [](const auto &dependency) {
                    return dependency.first.ends_with("unevaluatedItems");
                  })};

  return compiler_draft4_applicator_additionalitems_from_cursor(
      context, schema_context, dynamic_context, cursor,
      context.mode == Mode::Exhaustive,
      track && !schema_context.schema.defines("unevaluatedItems"));
}

auto compiler_2020_12_applicator_contains(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context,
                                          const Instructions &current)
    -> Instructions {
  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track{
      std::any_of(context.unevaluated.cbegin(), context.unevaluated.cend(),
                  [](const auto &dependency) {
                    return dependency.first.ends_with("unevaluatedItems");
                  })};

  return compiler_2019_09_applicator_contains_with_options(
      context, schema_context, dynamic_context, current,
      context.mode == Mode::Exhaustive, track);
}

auto compiler_2020_12_core_dynamicref(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &current)
    -> Instructions {
  const auto &entry{static_frame_entry(context, schema_context)};
  // In this case, just behave as a normal static reference
  if (!context.frame.references().contains(
          {sourcemeta::jsontoolkit::ReferenceType::Dynamic, entry.pointer})) {
    return compiler_draft4_core_ref(context, schema_context, dynamic_context,
                                    current);
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_string());
  sourcemeta::jsontoolkit::URI reference{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  reference.try_resolve_from(schema_context.base);
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
  return {make(sourcemeta::blaze::InstructionIndex::ControlDynamicAnchorJump,
               context, schema_context, dynamic_context,
               std::string{reference.fragment().value()})};
}

} // namespace internal
#endif
