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
                                relative_dynamic_context(dynamic_context),
                                sourcemeta::jsontoolkit::empty_pointer,
                                sourcemeta::jsontoolkit::empty_pointer)};

  // `then`
  std::size_t then_cursor{0};
  if (schema_context.schema.defines("then")) {
    then_cursor = children.size();
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat({"then"}),
               schema_context.base)
            .recompose()};
    assert(context.frame.locations().contains(
        {sourcemeta::jsontoolkit::ReferenceType::Static, destination}));
    DynamicContext new_dynamic_context{"then",
                                       dynamic_context.base_schema_location,
                                       sourcemeta::jsontoolkit::empty_pointer,
                                       dynamic_context.property_as_target};
    for (auto &&step :
         compile(context, schema_context, new_dynamic_context,
                 sourcemeta::jsontoolkit::empty_pointer,
                 sourcemeta::jsontoolkit::empty_pointer, destination)) {
      children.push_back(std::move(step));
    }

    // In this case, `if` did nothing, so we can short-circuit
    if (then_cursor == 0) {
      return children;
    }
  }

  // `else`
  std::size_t else_cursor{0};
  if (schema_context.schema.defines("else")) {
    else_cursor = children.size();
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat({"else"}),
               schema_context.base)
            .recompose()};
    assert(context.frame.locations().contains(
        {sourcemeta::jsontoolkit::ReferenceType::Static, destination}));
    DynamicContext new_dynamic_context{"else",
                                       dynamic_context.base_schema_location,
                                       sourcemeta::jsontoolkit::empty_pointer,
                                       dynamic_context.property_as_target};
    for (auto &&step :
         compile(context, schema_context, new_dynamic_context,
                 sourcemeta::jsontoolkit::empty_pointer,
                 sourcemeta::jsontoolkit::empty_pointer, destination)) {
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
