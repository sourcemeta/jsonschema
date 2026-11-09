#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT7_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT7_H_

#include <sourcemeta/blaze/compiler.h>

#include "compile_helpers.h"

namespace sourcemeta::blaze {

// TODO: Don't generate `if` if neither `then` nor `else` is defined
auto compiler_draft7_applicator_if(const KeywordContext &context,
                                   const Instructions &) -> Instructions {
  // `if`
  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  // `then`
  std::size_t then_cursor{children.size()};
  if (context.schema().defines("then")) {
    const auto destination{
        to_uri(context.local.relative_pointer.initial().concat(
                   make_weak_pointer(KEYWORD_THEN)),
               context.base())
            .recompose()};
    assert(context.global.frame
               .location(sourcemeta::core::SchemaReferenceType::Static,
                         destination)
               .has_value());

    // If `if` compiled to nothing, the `then` children will be hoisted to the
    // current level without the `LogicalCondition` wrapper. When the schema
    // uses dynamic scoping, we still need a wrapper instruction that keeps the
    // current schema resource on the evaluation path for dynamic anchor
    // resolution, as the omitted `LogicalCondition` instruction would have
    // otherwise done it
    if (then_cursor == 0 && context.global.uses_dynamic_scopes) {
      Instructions substeps{
          compile(context.relative(), sourcemeta::core::EMPTY_WEAK_POINTER,
                  sourcemeta::core::EMPTY_WEAK_POINTER, destination)};
      if (substeps.empty()) {
        return children;
      }

      const auto then_pointer{context.local.relative_pointer.initial().concat(
          make_weak_pointer(KEYWORD_THEN))};
      const SchemaContext then_schema_context{
          .relative_pointer = then_pointer,
          .schema = context.schema(),
          .vocabularies = context.vocabularies(),
          .base = context.base(),
          .is_property_name = context.local.is_property_name};
      const KeywordContext then_context{
          .global = context.global,
          .local = then_schema_context,
          .target = {
              .keyword = KEYWORD_THEN,
              .base_schema_location = context.target.base_schema_location,
              .base_instance_location = context.target.base_instance_location}};
      return {make(InstructionIndex::LogicalAnd, then_context, ValueNone{},
                   std::move(substeps))};
    }

    // When hoisting, the `then` children must carry the instance navigation
    // that the omitted `LogicalCondition` wrapper would have performed
    const KeywordContext then_context{
        .global = context.global,
        .local = context.local,
        .target = {.keyword = KEYWORD_THEN,
                   .base_schema_location = context.target.base_schema_location,
                   .base_instance_location =
                       then_cursor == 0
                           ? context.target.base_instance_location
                           : sourcemeta::core::EMPTY_WEAK_POINTER}};
    for (auto &&step :
         compile(then_context, sourcemeta::core::EMPTY_WEAK_POINTER,
                 sourcemeta::core::EMPTY_WEAK_POINTER, destination)) {
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
  if (context.schema().defines("else")) {
    else_cursor = children.size();
    const auto destination{
        to_uri(context.local.relative_pointer.initial().concat(
                   make_weak_pointer(KEYWORD_ELSE)),
               context.base())
            .recompose()};
    assert(context.global.frame
               .location(sourcemeta::core::SchemaReferenceType::Static,
                         destination)
               .has_value());
    const KeywordContext else_context{
        .global = context.global,
        .local = context.local,
        .target = {.keyword = KEYWORD_ELSE,
                   .base_schema_location = context.target.base_schema_location,
                   .base_instance_location =
                       sourcemeta::core::EMPTY_WEAK_POINTER}};
    for (auto &&step :
         compile(else_context, sourcemeta::core::EMPTY_WEAK_POINTER,
                 sourcemeta::core::EMPTY_WEAK_POINTER, destination)) {
      children.push_back(std::move(step));
    }
  }

  return {make(InstructionIndex::LogicalCondition, context,
               ValueIndexPair{then_cursor, else_cursor}, std::move(children))};
}

// We handle `then` as part of `if`
// TODO: Stop collapsing this keyword on exhaustive mode for debuggability
// purposes
auto compiler_draft7_applicator_then(const KeywordContext &,
                                     const Instructions &) -> Instructions {
  return {};
}

// We handle `else` as part of `if`
// TODO: Stop collapsing this keyword on exhaustive mode for debuggability
// purposes
auto compiler_draft7_applicator_else(const KeywordContext &,
                                     const Instructions &) -> Instructions {
  return {};
}

} // namespace sourcemeta::blaze
#endif
