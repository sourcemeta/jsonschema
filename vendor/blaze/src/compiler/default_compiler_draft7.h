#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT7_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT7_H_

#include <sourcemeta/blaze/compiler.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::blaze;

// TODO: Don't generate `if` if neither `then` nor `else` is defined
auto compiler_draft7_applicator_if(const Context &context,
                                   const SchemaContext &schema_context,
                                   const DynamicContext &dynamic_context,
                                   const Instructions &) -> Instructions {
  // `if`
  Instructions children{compile(context, schema_context,
                                relative_dynamic_context(),
                                sourcemeta::core::empty_weak_pointer,
                                sourcemeta::core::empty_weak_pointer)};

  // `then`
  std::size_t then_cursor{children.size()};
  if (schema_context.schema.defines("then")) {
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat(
                   make_weak_pointer(KEYWORD_THEN)),
               schema_context.base)
            .recompose()};
    assert(context.frame.locations().contains(
        {sourcemeta::blaze::SchemaReferenceType::Static, destination}));

    // If `if` compiled to nothing, the `then` children will be hoisted to the
    // current level without the `LogicalCondition` wrapper. When the schema
    // uses dynamic scoping, we still need a wrapper instruction that keeps the
    // current schema resource on the evaluation path for dynamic anchor
    // resolution, as the omitted `LogicalCondition` instruction would have
    // otherwise done it
    if (then_cursor == 0 && context.uses_dynamic_scopes) {
      Instructions substeps{
          compile(context, schema_context, relative_dynamic_context(),
                  sourcemeta::core::empty_weak_pointer,
                  sourcemeta::core::empty_weak_pointer, destination)};
      if (substeps.empty()) {
        return children;
      }

      const auto then_pointer{schema_context.relative_pointer.initial().concat(
          make_weak_pointer(KEYWORD_THEN))};
      const SchemaContext then_schema_context{
          .relative_pointer = then_pointer,
          .schema = schema_context.schema,
          .vocabularies = schema_context.vocabularies,
          .base = schema_context.base,
          .is_property_name = schema_context.is_property_name};
      return {make(
          sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
          then_schema_context,
          {.keyword = KEYWORD_THEN,
           .base_schema_location = dynamic_context.base_schema_location,
           .base_instance_location = dynamic_context.base_instance_location},
          ValueNone{}, std::move(substeps))};
    }

    // When hoisting, the `then` children must carry the instance navigation
    // that the omitted `LogicalCondition` wrapper would have performed
    DynamicContext new_dynamic_context{
        .keyword = KEYWORD_THEN,
        .base_schema_location = dynamic_context.base_schema_location,
        .base_instance_location = then_cursor == 0
                                      ? dynamic_context.base_instance_location
                                      : sourcemeta::core::empty_weak_pointer};
    for (auto &&step :
         compile(context, schema_context, new_dynamic_context,
                 sourcemeta::core::empty_weak_pointer,
                 sourcemeta::core::empty_weak_pointer, destination)) {
      children.push_back(std::move(step));
    }

    // In this case, `if` did nothing, so we can short-circuit
    if (then_cursor == 0) {
      return children;
    }
  }

  // If `if` compiled to nothing, it always succeeds, so in the absence of
  // `then`, the `else` subschema can never apply and the entire conditional
  // is a no-op
  if (then_cursor == 0) {
    assert(children.empty());
    return {};
  }

  // `else`
  std::size_t else_cursor{0};
  if (schema_context.schema.defines("else")) {
    else_cursor = children.size();
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat(
                   make_weak_pointer(KEYWORD_ELSE)),
               schema_context.base)
            .recompose()};
    assert(context.frame.locations().contains(
        {sourcemeta::blaze::SchemaReferenceType::Static, destination}));
    DynamicContext new_dynamic_context{
        .keyword = KEYWORD_ELSE,
        .base_schema_location = dynamic_context.base_schema_location,
        .base_instance_location = sourcemeta::core::empty_weak_pointer};
    for (auto &&step :
         compile(context, schema_context, new_dynamic_context,
                 sourcemeta::core::empty_weak_pointer,
                 sourcemeta::core::empty_weak_pointer, destination)) {
      children.push_back(std::move(step));
    }
  }

  return {make(sourcemeta::blaze::InstructionIndex::LogicalCondition, context,
               schema_context, dynamic_context,
               ValueIndexPair{then_cursor, else_cursor}, std::move(children))};
}

// We handle `then` as part of `if`
// TODO: Stop collapsing this keyword on exhaustive mode for debuggability
// purposes
auto compiler_draft7_applicator_then(const Context &, const SchemaContext &,
                                     const DynamicContext &,
                                     const Instructions &) -> Instructions {
  return {};
}

// We handle `else` as part of `if`
// TODO: Stop collapsing this keyword on exhaustive mode for debuggability
// purposes
auto compiler_draft7_applicator_else(const Context &, const SchemaContext &,
                                     const DynamicContext &,
                                     const Instructions &) -> Instructions {
  return {};
}

} // namespace internal
#endif
