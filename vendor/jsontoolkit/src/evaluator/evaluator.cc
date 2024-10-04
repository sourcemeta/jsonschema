#include <sourcemeta/jsontoolkit/evaluator.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "trace.h"

#include <algorithm> // std::min, std::any_of
#include <cassert>   // assert
#include <iterator>  // std::distance, std::advance
#include <limits>    // std::numeric_limits
#include <optional>  // std::optional

namespace {

auto evaluate_step(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const std::optional<
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback> &callback,
    sourcemeta::jsontoolkit::EvaluationContext &context) -> bool {
  SOURCEMETA_TRACE_REGISTER_ID(trace_dispatch_id);
  SOURCEMETA_TRACE_REGISTER_ID(trace_id);
  SOURCEMETA_TRACE_START(trace_dispatch_id, "Dispatch");
  using namespace sourcemeta::jsontoolkit;

#define STRINGIFY(x) #x

#define EVALUATE_BEGIN(step_category, step_type, precondition)                 \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  const auto &target{context.resolve_target()};                                \
  if (!(precondition)) {                                                       \
    context.pop(step_category.dynamic);                                        \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_IF_STRING(step_category, step_type)                     \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  const auto &maybe_target{context.resolve_string_target()};                   \
  if (!maybe_target.has_value()) {                                             \
    context.pop(step_category.dynamic);                                        \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  const auto &target{maybe_target.value().get()};                              \
  bool result{false};

#define EVALUATE_BEGIN_NO_TARGET(step_category, step_type, precondition)       \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  if (!(precondition)) {                                                       \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

  // This is a slightly complicated dance to avoid traversing the relative
  // instance location twice. We first need to traverse it to check if its
  // valid in the document as part of the condition, but if it is, we can
  // pass it to `.push()` so that it doesn't need to traverse it again.
#define EVALUATE_BEGIN_TRY_TARGET(step_category, step_type, precondition)      \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &target{context.resolve_target()};                                \
  const auto &step_category{std::get<step_type>(step)};                        \
  if (!(precondition)) {                                                       \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  auto target_check{                                                           \
      try_get(target, step_category.relative_instance_location)};              \
  if (!target_check.has_value()) {                                             \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic,           \
               std::move(target_check.value()));                               \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(step_category, step_type)               \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  bool result{false};

#define EVALUATE_END(step_category, step_type)                                 \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(SchemaCompilerEvaluationType::Post, result, step,         \
                     context.evaluate_path(), context.instance_location(),     \
                     context.null);                                            \
  }                                                                            \
  context.pop(step_category.dynamic);                                          \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return result;

  // As a safety guard, only emit the annotation if it didn't exist already.
  // Otherwise we risk confusing consumers

#define EVALUATE_ANNOTATION(step_category, step_type, precondition,            \
                            destination, annotation_value)                     \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  assert(step_category.relative_instance_location.empty());                    \
  const auto &target{context.resolve_target()};                                \
  if (!(precondition)) {                                                       \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  const auto annotation_result{                                                \
      context.annotate(destination, annotation_value)};                        \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  if (annotation_result.second && step_category.report &&                      \
      callback.has_value()) {                                                  \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), destination, context.null);      \
    callback.value()(SchemaCompilerEvaluationType::Post, true, step,           \
                     context.evaluate_path(), destination,                     \
                     annotation_result.first);                                 \
  }                                                                            \
  context.pop(step_category.dynamic);                                          \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return true;

#define EVALUATE_ANNOTATION_NO_PRECONDITION(step_category, step_type,          \
                                            destination, annotation_value)     \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  const auto annotation_result{                                                \
      context.annotate(destination, annotation_value)};                        \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic);          \
  if (annotation_result.second && step_category.report &&                      \
      callback.has_value()) {                                                  \
    callback.value()(SchemaCompilerEvaluationType::Pre, true, step,            \
                     context.evaluate_path(), destination, context.null);      \
    callback.value()(SchemaCompilerEvaluationType::Post, true, step,           \
                     context.evaluate_path(), destination,                     \
                     annotation_result.first);                                 \
  }                                                                            \
  context.pop(step_category.dynamic);                                          \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return true;

#define IS_STEP(step_type) SchemaCompilerTemplateIndex::step_type
  switch (static_cast<SchemaCompilerTemplateIndex>(step.index())) {
    case IS_STEP(SchemaCompilerAssertionFail): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionFail);
      EVALUATE_END(assertion, SchemaCompilerAssertionFail);
    }

    case IS_STEP(SchemaCompilerAssertionDefines): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDefines,
                     target.is_object());
      result = target.defines(assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionDefines);
    }

    case IS_STEP(SchemaCompilerAssertionDefinesAll): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDefinesAll,
                     target.is_object());

      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      result = true;
      for (const auto &property : assertion.value) {
        if (!target.defines(property)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(assertion, SchemaCompilerAssertionDefinesAll);
    }

    case IS_STEP(SchemaCompilerAssertionPropertyDependencies): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionPropertyDependencies,
                     target.is_object());
      // Otherwise we are we even emitting this instruction?
      assert(!assertion.value.empty());
      result = true;
      for (const auto &[property, dependencies] : assertion.value) {
        if (!target.defines(property)) {
          continue;
        }

        assert(!dependencies.empty());
        for (const auto &dependency : dependencies) {
          if (!target.defines(dependency)) {
            result = false;
            // For efficiently breaking from the outer loop too
            goto evaluate_assertion_property_dependencies_end;
          }
        }
      }

    evaluate_assertion_property_dependencies_end:
      EVALUATE_END(assertion, SchemaCompilerAssertionPropertyDependencies);
    }

    case IS_STEP(SchemaCompilerAssertionType): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionType);
      const auto &target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      result =
          target.type() == assertion.value ||
          (assertion.value == JSON::Type::Integer && target.is_integer_real());
      EVALUATE_END(assertion, SchemaCompilerAssertionType);
    }

    case IS_STEP(SchemaCompilerAssertionTypeAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionTypeAny);
      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      const auto &target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      for (const auto type : assertion.value) {
        if (type == JSON::Type::Integer && target.is_integer_real()) {
          result = true;
          break;
        } else if (type == target.type()) {
          result = true;
          break;
        }
      }

      EVALUATE_END(assertion, SchemaCompilerAssertionTypeAny);
    }

    case IS_STEP(SchemaCompilerAssertionTypeStrict): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionTypeStrict);
      result = context.resolve_target().type() == assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionTypeStrict);
    }

    case IS_STEP(SchemaCompilerAssertionTypeStrictAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionTypeStrictAny);
      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target().type()) !=
                assertion.value.cend());
      EVALUATE_END(assertion, SchemaCompilerAssertionTypeStrictAny);
    }

    case IS_STEP(SchemaCompilerAssertionTypeStringBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionTypeStringBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == JSON::Type::String &&
               target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, SchemaCompilerAssertionTypeStringBounded);
    }

    case IS_STEP(SchemaCompilerAssertionTypeArrayBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionTypeArrayBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == JSON::Type::Array && target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, SchemaCompilerAssertionTypeArrayBounded);
    }

    case IS_STEP(SchemaCompilerAssertionTypeObjectBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionTypeObjectBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == JSON::Type::Object &&
               target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, SchemaCompilerAssertionTypeObjectBounded);
    }

    case IS_STEP(SchemaCompilerAssertionRegex): {
      EVALUATE_BEGIN_IF_STRING(assertion, SchemaCompilerAssertionRegex);
      result = std::regex_search(target, assertion.value.first);
      EVALUATE_END(assertion, SchemaCompilerAssertionRegex);
    }

    case IS_STEP(SchemaCompilerAssertionStringSizeLess): {
      EVALUATE_BEGIN_IF_STRING(assertion,
                               SchemaCompilerAssertionStringSizeLess);
      result = (JSON::size(target) < assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionStringSizeLess);
    }

    case IS_STEP(SchemaCompilerAssertionStringSizeGreater): {
      EVALUATE_BEGIN_IF_STRING(assertion,
                               SchemaCompilerAssertionStringSizeGreater);
      result = (JSON::size(target) > assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionStringSizeGreater);
    }

    case IS_STEP(SchemaCompilerAssertionArraySizeLess): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionArraySizeLess,
                     target.is_array());
      result = (target.size() < assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionArraySizeLess);
    }

    case IS_STEP(SchemaCompilerAssertionArraySizeGreater): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionArraySizeGreater,
                     target.is_array());
      result = (target.size() > assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionArraySizeGreater);
    }

    case IS_STEP(SchemaCompilerAssertionObjectSizeLess): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionObjectSizeLess,
                     target.is_object());
      result = (target.size() < assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionObjectSizeLess);
    }

    case IS_STEP(SchemaCompilerAssertionObjectSizeGreater): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionObjectSizeGreater,
                     target.is_object());
      result = (target.size() > assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionObjectSizeGreater);
    }

    case IS_STEP(SchemaCompilerAssertionEqual): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, SchemaCompilerAssertionEqual);
      result = (context.resolve_target() == assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionEqual);
    }

    case IS_STEP(SchemaCompilerAssertionEqualsAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion,
                                     SchemaCompilerAssertionEqualsAny);
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target()) != assertion.value.cend());
      EVALUATE_END(assertion, SchemaCompilerAssertionEqualsAny);
    }

    case IS_STEP(SchemaCompilerAssertionGreaterEqual): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionGreaterEqual,
                     target.is_number());
      result = target >= assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionGreaterEqual);
    }

    case IS_STEP(SchemaCompilerAssertionLessEqual): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionLessEqual,
                     target.is_number());
      result = target <= assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionLessEqual);
    }

    case IS_STEP(SchemaCompilerAssertionGreater): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionGreater,
                     target.is_number());
      result = target > assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionGreater);
    }

    case IS_STEP(SchemaCompilerAssertionLess): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionLess,
                     target.is_number());
      result = target < assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionLess);
    }

    case IS_STEP(SchemaCompilerAssertionUnique): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionUnique,
                     target.is_array());
      result = target.unique();
      EVALUATE_END(assertion, SchemaCompilerAssertionUnique);
    }

    case IS_STEP(SchemaCompilerAssertionDivisible): {
      EVALUATE_BEGIN(assertion, SchemaCompilerAssertionDivisible,
                     target.is_number());
      assert(assertion.value.is_number());
      result = target.divisible_by(assertion.value);
      EVALUATE_END(assertion, SchemaCompilerAssertionDivisible);
    }

    case IS_STEP(SchemaCompilerAssertionStringType): {
      EVALUATE_BEGIN_IF_STRING(assertion, SchemaCompilerAssertionStringType);
      switch (assertion.value) {
        case SchemaCompilerValueStringType::URI:
          try {
            // TODO: This implies a string copy
            result = URI{target}.is_absolute();
          } catch (const URIParseError &) {
            result = false;
          }

          break;
        default:
          // We should never get here
          assert(false);
      }

      EVALUATE_END(assertion, SchemaCompilerAssertionStringType);
    }

    case IS_STEP(SchemaCompilerAssertionPropertyType): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, SchemaCompilerAssertionPropertyType,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      const auto &effective_target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      result = effective_target.type() == assertion.value ||
               (assertion.value == JSON::Type::Integer &&
                effective_target.is_integer_real());
      EVALUATE_END(assertion, SchemaCompilerAssertionPropertyType);
    }

    case IS_STEP(SchemaCompilerAssertionPropertyTypeStrict): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, SchemaCompilerAssertionPropertyTypeStrict,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      result = context.resolve_target().type() == assertion.value;
      EVALUATE_END(assertion, SchemaCompilerAssertionPropertyTypeStrict);
    }

    case IS_STEP(SchemaCompilerLogicalOr): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalOr);
      result = logical.children.empty();
      for (const auto &child : logical.children) {
        if (evaluate_step(child, mode, callback, context)) {
          result = true;
          // This boolean value controls whether we should still evaluate
          // every disjunction even on fast mode
          if (mode == SchemaCompilerEvaluationMode::Fast && !logical.value) {
            break;
          }
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalOr);
    }

    case IS_STEP(SchemaCompilerLogicalAnd): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalAnd);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalAnd);
    }

    case IS_STEP(SchemaCompilerLogicalWhenType): {
      EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenType,
                     target.type() == logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalWhenType);
    }

    case IS_STEP(SchemaCompilerLogicalWhenDefines): {
      EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenDefines,
                     target.is_object() && target.defines(logical.value));
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalWhenDefines);
    }

    case IS_STEP(SchemaCompilerLogicalWhenArraySizeGreater): {
      EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenArraySizeGreater,
                     target.is_array() && target.size() > logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalWhenArraySizeGreater);
    }

    case IS_STEP(SchemaCompilerLogicalWhenArraySizeEqual): {
      EVALUATE_BEGIN(logical, SchemaCompilerLogicalWhenArraySizeEqual,
                     target.is_array() && target.size() == logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalWhenArraySizeEqual);
    }

    case IS_STEP(SchemaCompilerLogicalXor): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalXor);
      result = true;
      bool has_matched{false};
      for (const auto &child : logical.children) {
        if (evaluate_step(child, mode, callback, context)) {
          if (has_matched) {
            result = false;
            if (mode == SchemaCompilerEvaluationMode::Fast) {
              break;
            }
          } else {
            has_matched = true;
          }
        }
      }

      result = result && has_matched;
      EVALUATE_END(logical, SchemaCompilerLogicalXor);
    }

    case IS_STEP(SchemaCompilerLogicalCondition): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalCondition);
      result = true;
      const auto children_size{logical.children.size()};
      assert(children_size >= logical.value.first);
      assert(children_size >= logical.value.second);

      auto condition_end{children_size};
      if (logical.value.first > 0) {
        condition_end = logical.value.first;
      } else if (logical.value.second > 0) {
        condition_end = logical.value.second;
      }

      for (std::size_t cursor = 0; cursor < condition_end; cursor++) {
        if (!evaluate_step(logical.children[cursor], mode, callback, context)) {
          result = false;
          break;
        }
      }

      const auto consequence_start{result ? logical.value.first
                                          : logical.value.second};
      const auto consequence_end{(result && logical.value.second > 0)
                                     ? logical.value.second
                                     : children_size};
      result = true;
      if (consequence_start > 0) {
        for (auto cursor = consequence_start; cursor < consequence_end;
             cursor++) {
          if (!evaluate_step(logical.children[cursor], mode, callback,
                             context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalCondition);
    }

    case IS_STEP(SchemaCompilerLogicalNot): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, SchemaCompilerLogicalNot);
      // Ignore annotations produced inside "not"
      context.mask();
      result = false;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = true;
          if (mode == SchemaCompilerEvaluationMode::Fast) {
            break;
          }
        }
      }

      EVALUATE_END(logical, SchemaCompilerLogicalNot);
    }

    case IS_STEP(SchemaCompilerControlLabel): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, SchemaCompilerControlLabel);
      context.mark(control.value, control.children);
      result = true;
      for (const auto &child : control.children) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(control, SchemaCompilerControlLabel);
    }

    case IS_STEP(SchemaCompilerControlMark): {
      SOURCEMETA_TRACE_START(trace_id, "SchemaCompilerControlMark");
      const auto &control{std::get<SchemaCompilerControlMark>(step)};
      context.mark(control.value, control.children);
      SOURCEMETA_TRACE_END(trace_id, "SchemaCompilerControlMark");
      return true;
    }

    case IS_STEP(SchemaCompilerControlJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, SchemaCompilerControlJump);
      result = true;
      for (const auto &child : context.jump(control.value)) {
        if (!evaluate_step(child, mode, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(control, SchemaCompilerControlJump);
    }

    case IS_STEP(SchemaCompilerControlDynamicAnchorJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control,
                                     SchemaCompilerControlDynamicAnchorJump);
      const auto id{context.find_dynamic_anchor(control.value)};
      result = id.has_value();
      if (id.has_value()) {
        for (const auto &child : context.jump(id.value())) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(control, SchemaCompilerControlDynamicAnchorJump);
    }

    case IS_STEP(SchemaCompilerAnnotationEmit): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(
          annotation, SchemaCompilerAnnotationEmit, context.instance_location(),
          annotation.value);
    }

    case IS_STEP(SchemaCompilerAnnotationWhenArraySizeEqual): {
      EVALUATE_ANNOTATION(
          annotation, SchemaCompilerAnnotationWhenArraySizeEqual,
          target.is_array() && target.size() == annotation.value.first,
          context.instance_location(), annotation.value.second);
    }

    case IS_STEP(SchemaCompilerAnnotationWhenArraySizeGreater): {
      EVALUATE_ANNOTATION(
          annotation, SchemaCompilerAnnotationWhenArraySizeGreater,
          target.is_array() && target.size() > annotation.value.first,
          context.instance_location(), annotation.value.second);
    }

    case IS_STEP(SchemaCompilerAnnotationToParent): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(
          annotation, SchemaCompilerAnnotationToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location().initial(), annotation.value);
    }

    case IS_STEP(SchemaCompilerAnnotationBasenameToParent): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(
          annotation, SchemaCompilerAnnotationBasenameToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location().initial(),
          context.instance_location().back().to_json());
    }

    case IS_STEP(SchemaCompilerLoopPropertiesMatch): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesMatch,
                     target.is_object());
      assert(!loop.value.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        const auto index{loop.value.find(entry.first)};
        if (index == loop.value.cend()) {
          continue;
        }

        const auto &substep{loop.children[index->second]};
        assert(std::holds_alternative<SchemaCompilerLogicalAnd>(substep));
        for (const auto &child :
             std::get<SchemaCompilerLogicalAnd>(substep).children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_match_end;
          }
        }
      }

    evaluate_loop_properties_match_end:
      EVALUATE_END(loop, SchemaCompilerLoopPropertiesMatch);
    }

    case IS_STEP(SchemaCompilerLoopProperties): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopProperties, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_end:
      EVALUATE_END(loop, SchemaCompilerLoopProperties);
    }

    case IS_STEP(SchemaCompilerLoopPropertiesRegex): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesRegex,
                     target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (!std::regex_search(entry.first, loop.value.first)) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_regex_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_regex_end:
      EVALUATE_END(loop, SchemaCompilerLoopPropertiesRegex);
    }

    case IS_STEP(SchemaCompilerLoopPropertiesNoAnnotation): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesNoAnnotation,
                     target.is_object());
      result = true;
      assert(!loop.value.empty());

      for (const auto &entry : target.as_object()) {
        // TODO: It might be more efficient to get all the annotations we
        // potentially care about as a set first, and the make the loop
        // check for O(1) containment in that set?
        if (context.defines_annotation(
                context.instance_location(),
                // TODO: Can we avoid doing this expensive operation on a loop?
                context.evaluate_path().initial(), loop.value,
                // TODO: This conversion implies a string copy
                JSON{entry.first})) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_no_annotation_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_no_annotation_end:
      EVALUATE_END(loop, SchemaCompilerLoopPropertiesNoAnnotation);
    }

    case IS_STEP(SchemaCompilerLoopPropertiesExcept): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesExcept,
                     target.is_object());
      result = true;
      // Otherwise why emit this instruction?
      assert(!loop.value.first.empty() || !loop.value.second.empty());

      for (const auto &entry : target.as_object()) {
        if (loop.value.first.contains(entry.first) ||
            std::any_of(loop.value.second.cbegin(), loop.value.second.cend(),
                        [&entry](const auto &pattern) {
                          return std::regex_search(entry.first, pattern.first);
                        })) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_except_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_except_end:
      EVALUATE_END(loop, SchemaCompilerLoopPropertiesExcept);
    }

    case IS_STEP(SchemaCompilerLoopPropertiesType): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesType,
                     target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        const auto &entry_target{context.resolve_target()};
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        if (entry_target.type() != loop.value &&
            (loop.value != JSON::Type::Integer ||
             entry_target.is_integer_real())) {
          result = false;
          context.leave();
          break;
        }

        context.leave();
      }

      EVALUATE_END(loop, SchemaCompilerLoopPropertiesType);
    }

    case IS_STEP(SchemaCompilerLoopPropertiesTypeStrict): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopPropertiesTypeStrict,
                     target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        if (context.resolve_target().type() != loop.value) {
          result = false;
          context.leave();
          break;
        }

        context.leave();
      }

      EVALUATE_END(loop, SchemaCompilerLoopPropertiesTypeStrict);
    }

    case IS_STEP(SchemaCompilerLoopKeys): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopKeys, target.is_object());
      result = true;
      context.target_type(
          sourcemeta::jsontoolkit::EvaluationContext::TargetType::Key);
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_loop_keys_end;
          }
        }

        context.leave();
      }

    evaluate_loop_keys_end:
      context.target_type(
          sourcemeta::jsontoolkit::EvaluationContext::TargetType::Value);
      EVALUATE_END(loop, SchemaCompilerLoopKeys);
    }

    case IS_STEP(SchemaCompilerLoopItems): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopItems, target.is_array());
      const auto &array{target.as_array()};
      result = true;
      auto iterator{array.cbegin()};

      // We need this check, as advancing an iterator past its bounds
      // is considered undefined behavior
      // See https://en.cppreference.com/w/cpp/iterator/advance
      std::advance(iterator,
                   std::min(static_cast<std::ptrdiff_t>(loop.value),
                            static_cast<std::ptrdiff_t>(target.size())));

      for (; iterator != array.cend(); ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        context.enter(static_cast<Pointer::Token::Index>(index));
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_loop_items_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_loop_items_end:
      EVALUATE_END(loop, SchemaCompilerLoopItems);
    }

    case IS_STEP(SchemaCompilerLoopItemsUnmarked): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopItemsUnmarked,
                     target.is_array() &&
                         !context.defines_annotation(
                             context.instance_location(),
                             context.evaluate_path(), loop.value, JSON{true}));
      // Otherwise you shouldn't be using this step?
      assert(!loop.value.empty());
      const auto &array{target.as_array()};
      result = true;

      for (auto iterator = array.cbegin(); iterator != array.cend();
           ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        context.enter(static_cast<Pointer::Token::Index>(index));
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_loop_items_unmarked_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_loop_items_unmarked_end:
      EVALUATE_END(loop, SchemaCompilerLoopItemsUnmarked);
    }

    case IS_STEP(SchemaCompilerLoopItemsUnevaluated): {
      // TODO: This precondition is very expensive due to pointer manipulation
      EVALUATE_BEGIN(loop, SchemaCompilerLoopItemsUnevaluated,
                     target.is_array() && !context.defines_annotation(
                                              context.instance_location(),
                                              context.evaluate_path().initial(),
                                              loop.value.mask, JSON{true}));
      const auto &array{target.as_array()};
      result = true;
      auto iterator{array.cbegin()};

      // Determine the proper start based on integer annotations collected for
      // the current instance location by the keyword requested by the user.
      const std::uint64_t start{context.largest_annotation_index(
          context.instance_location(), {loop.value.index}, 0)};

      // We need this check, as advancing an iterator past its bounds
      // is considered undefined behavior
      // See https://en.cppreference.com/w/cpp/iterator/advance
      std::advance(iterator,
                   std::min(static_cast<std::ptrdiff_t>(start),
                            static_cast<std::ptrdiff_t>(target.size())));

      for (; iterator != array.cend(); ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};

        if (context.defines_annotation(
                context.instance_location(),
                // TODO: Can we avoid doing this expensive operation on a loop?
                context.evaluate_path().initial(), loop.value.filter,
                JSON{static_cast<std::size_t>(index)})) {
          continue;
        }

        context.enter(static_cast<Pointer::Token::Index>(index));
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_loop_items_unevaluated_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_loop_items_unevaluated_end:
      EVALUATE_END(loop, SchemaCompilerLoopItemsUnevaluated);
    }

    case IS_STEP(SchemaCompilerLoopContains): {
      EVALUATE_BEGIN(loop, SchemaCompilerLoopContains, target.is_array());
      const auto minimum{std::get<0>(loop.value)};
      const auto &maximum{std::get<1>(loop.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      const auto is_exhaustive{std::get<2>(loop.value)};
      result = minimum == 0 && target.empty();
      const auto &array{target.as_array()};
      auto match_count{std::numeric_limits<decltype(minimum)>::min()};
      for (auto iterator = array.cbegin(); iterator != array.cend();
           ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        context.enter(static_cast<Pointer::Token::Index>(index));
        bool subresult{true};
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, mode, callback, context)) {
            subresult = false;
            break;
          }
        }

        context.leave();

        if (subresult) {
          match_count += 1;

          // Exceeding the upper bound is definitely a failure
          if (maximum.has_value() && match_count > maximum.value()) {
            result = false;

            // Note that here we don't want to consider whether to run
            // exhaustively or not. At this point, its already a failure,
            // and anything that comes after would not run at all anyway
            break;
          }

          if (match_count >= minimum) {
            result = true;

            // Exceeding the lower bound when there is no upper bound
            // is definitely a success
            if (!maximum.has_value() && !is_exhaustive) {
              break;
            }
          }
        }
      }

      EVALUATE_END(loop, SchemaCompilerLoopContains);
    }

#undef IS_STEP
#undef STRINGIFY
#undef EVALUATE_BEGIN
#undef EVALUATE_BEGIN_IF_STRING
#undef EVALUATE_BEGIN_NO_TARGET
#undef EVALUATE_BEGIN_TRY_TARGET
#undef EVALUATE_BEGIN_NO_PRECONDITION
#undef EVALUATE_END
#undef EVALUATE_ANNOTATION
#undef EVALUATE_ANNOTATION_NO_PRECONDITION

    default:
      // We should never get here
      assert(false);
      return false;
  }
}

inline auto evaluate_internal(
    sourcemeta::jsontoolkit::EvaluationContext &context,
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate &steps,
    const sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode mode,
    const std::optional<
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback> &callback)
    -> bool {
  bool overall{true};
  for (const auto &step : steps) {
    if (!evaluate_step(step, mode, callback, context)) {
      overall = false;
      break;
    }
  }

  // The evaluation path and instance location must be empty by the time
  // we are done, otherwise there was a frame push/pop mismatch
  assert(context.evaluate_path().empty());
  assert(context.instance_location().empty());
  assert(context.resources().empty());
  // We should end up at the root of the instance
  assert(context.instances().size() == 1);
  return overall;
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto evaluate(const SchemaCompilerTemplate &steps, const JSON &instance,
              const SchemaCompilerEvaluationMode mode,
              const SchemaCompilerEvaluationCallback &callback) -> bool {
  EvaluationContext context;
  context.prepare(instance);
  return evaluate_internal(context, steps, mode, callback);
}

auto evaluate(const SchemaCompilerTemplate &steps, const JSON &instance)
    -> bool {
  EvaluationContext context;
  context.prepare(instance);
  return evaluate_internal(context, steps,
                           // Otherwise what's the point of an exhaustive
                           // evaluation if you don't get the results?
                           SchemaCompilerEvaluationMode::Fast, std::nullopt);
}

auto evaluate(const SchemaCompilerTemplate &steps, EvaluationContext &context)
    -> bool {
  return evaluate_internal(context, steps,
                           // Otherwise what's the point of an exhaustive
                           // evaluation if you don't get the results?
                           SchemaCompilerEvaluationMode::Fast, std::nullopt);
}

} // namespace sourcemeta::jsontoolkit
