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
    const DynamicContext &dynamic_context) -> Template {
  return compiler_draft4_applicator_items_array(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive, context.uses_unevaluated_items);
}

auto compiler_2020_12_applicator_items(const Context &context,
                                       const SchemaContext &schema_context,
                                       const DynamicContext &dynamic_context)
    -> Template {
  const auto cursor{(schema_context.schema.defines("prefixItems") &&
                     schema_context.schema.at("prefixItems").is_array())
                        ? schema_context.schema.at("prefixItems").size()
                        : 0};

  return compiler_draft4_applicator_additionalitems_from_cursor(
      context, schema_context, dynamic_context, cursor,
      context.mode == Mode::Exhaustive, context.uses_unevaluated_items);
}

auto compiler_2020_12_applicator_contains(const Context &context,
                                          const SchemaContext &schema_context,
                                          const DynamicContext &dynamic_context)
    -> Template {
  return compiler_2019_09_applicator_contains_with_options(
      context, schema_context, dynamic_context,
      context.mode == Mode::Exhaustive, context.uses_unevaluated_items);
}

auto compiler_2020_12_core_dynamicref(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  const auto current{
      to_uri(schema_context.relative_pointer, schema_context.base).recompose()};
  assert(context.frame.contains(
      {sourcemeta::jsontoolkit::ReferenceType::Static, current}));
  const auto &entry{context.frame.at(
      {sourcemeta::jsontoolkit::ReferenceType::Static, current})};
  // In this case, just behave as a normal static reference
  if (!context.references.contains(
          {sourcemeta::jsontoolkit::ReferenceType::Dynamic, entry.pointer})) {
    return compiler_draft4_core_ref(context, schema_context, dynamic_context);
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_string());
  sourcemeta::jsontoolkit::URI reference{
      schema_context.schema.at(dynamic_context.keyword).to_string()};
  reference.resolve_from_if_absolute(schema_context.base);
  reference.canonicalize();
  // We handle the non-anchor variant by not treating it as a dynamic reference
  assert(reference.fragment().has_value());

  // Note we don't need to even care about the static part of the dynamic
  // reference (if any), as even if we jump first there, we will still
  // look for the oldest dynamic anchor in the schema resource chain.
  return {make<ControlDynamicAnchorJump>(
      true, context, schema_context, dynamic_context,
      std::string{reference.fragment().value()})};
}

} // namespace internal
#endif
