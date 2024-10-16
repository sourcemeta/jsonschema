#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "trace.h"

#include <algorithm> // std::min, std::any_of, std::find
#include <cassert>   // assert
#include <iterator>  // std::distance, std::advance
#include <limits>    // std::numeric_limits
#include <optional>  // std::optional

namespace {

auto evaluate_step(const sourcemeta::blaze::Template::value_type &step,
                   const std::optional<sourcemeta::blaze::Callback> &callback,
                   sourcemeta::blaze::EvaluationContext &context) -> bool {
  SOURCEMETA_TRACE_REGISTER_ID(trace_dispatch_id);
  SOURCEMETA_TRACE_REGISTER_ID(trace_id);
  SOURCEMETA_TRACE_START(trace_dispatch_id, "Dispatch");
  using namespace sourcemeta::jsontoolkit;
  using namespace sourcemeta::blaze;

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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     context.instance_location(), context.null);               \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     context.instance_location(), context.null);               \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     context.instance_location(), context.null);               \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     context.instance_location(), context.null);               \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     context.instance_location(), context.null);               \
  }                                                                            \
  bool result{false};

#define EVALUATE_END(step_category, step_type)                                 \
  if (step_category.report && callback.has_value()) {                          \
    callback.value()(EvaluationType::Post, result, step,                       \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     destination, context.null);                               \
    callback.value()(EvaluationType::Post, true, step,                         \
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
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path(), \
                     destination, context.null);                               \
    callback.value()(EvaluationType::Post, true, step,                         \
                     context.evaluate_path(), destination,                     \
                     annotation_result.first);                                 \
  }                                                                            \
  context.pop(step_category.dynamic);                                          \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return true;

#define IS_STEP(step_type) TemplateIndex::step_type
  switch (static_cast<TemplateIndex>(step.index())) {
    case IS_STEP(AssertionFail): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionFail);
      EVALUATE_END(assertion, AssertionFail);
    }

    case IS_STEP(AssertionDefines): {
      EVALUATE_BEGIN(assertion, AssertionDefines, target.is_object());
      result = target.defines(assertion.value);
      EVALUATE_END(assertion, AssertionDefines);
    }

    case IS_STEP(AssertionDefinesAll): {
      EVALUATE_BEGIN(assertion, AssertionDefinesAll, target.is_object());

      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      result = true;
      for (const auto &property : assertion.value) {
        if (!target.defines(property)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(assertion, AssertionDefinesAll);
    }

    case IS_STEP(AssertionPropertyDependencies): {
      EVALUATE_BEGIN(assertion, AssertionPropertyDependencies,
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
      EVALUATE_END(assertion, AssertionPropertyDependencies);
    }

    case IS_STEP(AssertionType): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionType);
      const auto &target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      result =
          target.type() == assertion.value ||
          (assertion.value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
           target.is_integer_real());
      EVALUATE_END(assertion, AssertionType);
    }

    case IS_STEP(AssertionTypeAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeAny);
      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      const auto &target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      for (const auto type : assertion.value) {
        if (type == sourcemeta::jsontoolkit::JSON::Type::Integer &&
            target.is_integer_real()) {
          result = true;
          break;
        } else if (type == target.type()) {
          result = true;
          break;
        }
      }

      EVALUATE_END(assertion, AssertionTypeAny);
    }

    case IS_STEP(AssertionTypeStrict): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeStrict);
      result = context.resolve_target().type() == assertion.value;
      EVALUATE_END(assertion, AssertionTypeStrict);
    }

    case IS_STEP(AssertionTypeStrictAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeStrictAny);
      // Otherwise we are we even emitting this instruction?
      assert(assertion.value.size() > 1);
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target().type()) !=
                assertion.value.cend());
      EVALUATE_END(assertion, AssertionTypeStrictAny);
    }

    case IS_STEP(AssertionTypeStringBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeStringBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == sourcemeta::jsontoolkit::JSON::Type::String &&
               target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, AssertionTypeStringBounded);
    }

    case IS_STEP(AssertionTypeArrayBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeArrayBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == sourcemeta::jsontoolkit::JSON::Type::Array &&
               target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, AssertionTypeArrayBounded);
    }

    case IS_STEP(AssertionTypeObjectBounded): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionTypeObjectBounded);
      const auto &target{context.resolve_target()};
      const auto minimum{std::get<0>(assertion.value)};
      const auto maximum{std::get<1>(assertion.value)};
      assert(!maximum.has_value() || maximum.value() >= minimum);
      // Require early breaking
      assert(!std::get<2>(assertion.value));
      result = target.type() == sourcemeta::jsontoolkit::JSON::Type::Object &&
               target.size() >= minimum &&
               (!maximum.has_value() || target.size() <= maximum.value());
      EVALUATE_END(assertion, AssertionTypeObjectBounded);
    }

    case IS_STEP(AssertionRegex): {
      EVALUATE_BEGIN_IF_STRING(assertion, AssertionRegex);
      result = std::regex_search(target, assertion.value.first);
      EVALUATE_END(assertion, AssertionRegex);
    }

    case IS_STEP(AssertionStringSizeLess): {
      EVALUATE_BEGIN_IF_STRING(assertion, AssertionStringSizeLess);
      result = (sourcemeta::jsontoolkit::JSON::size(target) < assertion.value);
      EVALUATE_END(assertion, AssertionStringSizeLess);
    }

    case IS_STEP(AssertionStringSizeGreater): {
      EVALUATE_BEGIN_IF_STRING(assertion, AssertionStringSizeGreater);
      result = (sourcemeta::jsontoolkit::JSON::size(target) > assertion.value);
      EVALUATE_END(assertion, AssertionStringSizeGreater);
    }

    case IS_STEP(AssertionArraySizeLess): {
      EVALUATE_BEGIN(assertion, AssertionArraySizeLess, target.is_array());
      result = (target.size() < assertion.value);
      EVALUATE_END(assertion, AssertionArraySizeLess);
    }

    case IS_STEP(AssertionArraySizeGreater): {
      EVALUATE_BEGIN(assertion, AssertionArraySizeGreater, target.is_array());
      result = (target.size() > assertion.value);
      EVALUATE_END(assertion, AssertionArraySizeGreater);
    }

    case IS_STEP(AssertionObjectSizeLess): {
      EVALUATE_BEGIN(assertion, AssertionObjectSizeLess, target.is_object());
      result = (target.size() < assertion.value);
      EVALUATE_END(assertion, AssertionObjectSizeLess);
    }

    case IS_STEP(AssertionObjectSizeGreater): {
      EVALUATE_BEGIN(assertion, AssertionObjectSizeGreater, target.is_object());
      result = (target.size() > assertion.value);
      EVALUATE_END(assertion, AssertionObjectSizeGreater);
    }

    case IS_STEP(AssertionEqual): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionEqual);
      result = (context.resolve_target() == assertion.value);
      EVALUATE_END(assertion, AssertionEqual);
    }

    case IS_STEP(AssertionEqualsAny): {
      EVALUATE_BEGIN_NO_PRECONDITION(assertion, AssertionEqualsAny);
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target()) != assertion.value.cend());
      EVALUATE_END(assertion, AssertionEqualsAny);
    }

    case IS_STEP(AssertionGreaterEqual): {
      EVALUATE_BEGIN(assertion, AssertionGreaterEqual, target.is_number());
      result = target >= assertion.value;
      EVALUATE_END(assertion, AssertionGreaterEqual);
    }

    case IS_STEP(AssertionLessEqual): {
      EVALUATE_BEGIN(assertion, AssertionLessEqual, target.is_number());
      result = target <= assertion.value;
      EVALUATE_END(assertion, AssertionLessEqual);
    }

    case IS_STEP(AssertionGreater): {
      EVALUATE_BEGIN(assertion, AssertionGreater, target.is_number());
      result = target > assertion.value;
      EVALUATE_END(assertion, AssertionGreater);
    }

    case IS_STEP(AssertionLess): {
      EVALUATE_BEGIN(assertion, AssertionLess, target.is_number());
      result = target < assertion.value;
      EVALUATE_END(assertion, AssertionLess);
    }

    case IS_STEP(AssertionUnique): {
      EVALUATE_BEGIN(assertion, AssertionUnique, target.is_array());
      result = target.unique();
      EVALUATE_END(assertion, AssertionUnique);
    }

    case IS_STEP(AssertionDivisible): {
      EVALUATE_BEGIN(assertion, AssertionDivisible, target.is_number());
      assert(assertion.value.is_number());
      result = target.divisible_by(assertion.value);
      EVALUATE_END(assertion, AssertionDivisible);
    }

    case IS_STEP(AssertionStringType): {
      EVALUATE_BEGIN_IF_STRING(assertion, AssertionStringType);
      switch (assertion.value) {
        case ValueStringType::URI:
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

      EVALUATE_END(assertion, AssertionStringType);
    }

    case IS_STEP(AssertionPropertyType): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyType,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      const auto &effective_target{context.resolve_target()};
      // In non-strict mode, we consider a real number that represents an
      // integer to be an integer
      result =
          effective_target.type() == assertion.value ||
          (assertion.value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
           effective_target.is_integer_real());
      EVALUATE_END(assertion, AssertionPropertyType);
    }

    case IS_STEP(AssertionPropertyTypeStrict): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyTypeStrict,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      result = context.resolve_target().type() == assertion.value;
      EVALUATE_END(assertion, AssertionPropertyTypeStrict);
    }

    case IS_STEP(LogicalOr): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalOr);
      result = logical.children.empty();
      for (const auto &child : logical.children) {
        if (evaluate_step(child, callback, context)) {
          result = true;
          // This boolean value controls whether we should be exhaustive
          if (!logical.value) {
            break;
          }
        }
      }

      EVALUATE_END(logical, LogicalOr);
    }

    case IS_STEP(LogicalAnd): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalAnd);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, LogicalAnd);
    }

    case IS_STEP(LogicalWhenType): {
      EVALUATE_BEGIN(logical, LogicalWhenType, target.type() == logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, LogicalWhenType);
    }

    case IS_STEP(LogicalWhenDefines): {
      EVALUATE_BEGIN(logical, LogicalWhenDefines,
                     target.is_object() && target.defines(logical.value));
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, LogicalWhenDefines);
    }

    case IS_STEP(LogicalWhenArraySizeGreater): {
      EVALUATE_BEGIN(logical, LogicalWhenArraySizeGreater,
                     target.is_array() && target.size() > logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, LogicalWhenArraySizeGreater);
    }

    case IS_STEP(LogicalWhenArraySizeEqual): {
      EVALUATE_BEGIN(logical, LogicalWhenArraySizeEqual,
                     target.is_array() && target.size() == logical.value);
      result = true;
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(logical, LogicalWhenArraySizeEqual);
    }

    case IS_STEP(LogicalXor): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalXor);
      result = true;
      bool has_matched{false};
      for (const auto &child : logical.children) {
        if (evaluate_step(child, callback, context)) {
          if (has_matched) {
            result = false;
            // This boolean value controls whether we should be exhaustive
            if (!logical.value) {
              break;
            }
          } else {
            has_matched = true;
          }
        }
      }

      result = result && has_matched;
      EVALUATE_END(logical, LogicalXor);
    }

    case IS_STEP(LogicalCondition): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalCondition);
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
        if (!evaluate_step(logical.children[cursor], callback, context)) {
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
          if (!evaluate_step(logical.children[cursor], callback, context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(logical, LogicalCondition);
    }

    case IS_STEP(ControlLabel): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlLabel);
      context.mark(control.value, control.children);
      result = true;
      for (const auto &child : control.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(control, ControlLabel);
    }

    case IS_STEP(ControlMark): {
      SOURCEMETA_TRACE_START(trace_id, "ControlMark");
      const auto &control{std::get<ControlMark>(step)};
      context.mark(control.value, control.children);
      SOURCEMETA_TRACE_END(trace_id, "ControlMark");
      return true;
    }

    case IS_STEP(ControlJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlJump);
      result = true;
      for (const auto &child : context.jump(control.value)) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(control, ControlJump);
    }

    case IS_STEP(ControlDynamicAnchorJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlDynamicAnchorJump);
      const auto id{context.find_dynamic_anchor(control.value)};
      result = id.has_value();
      if (id.has_value()) {
        for (const auto &child : context.jump(id.value())) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(control, ControlDynamicAnchorJump);
    }

    case IS_STEP(AnnotationEmit): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(annotation, AnnotationEmit,
                                          context.instance_location(),
                                          annotation.value);
    }

    case IS_STEP(AnnotationWhenArraySizeEqual): {
      EVALUATE_ANNOTATION(annotation, AnnotationWhenArraySizeEqual,
                          target.is_array() &&
                              target.size() == annotation.value.first,
                          context.instance_location(), annotation.value.second);
    }

    case IS_STEP(AnnotationWhenArraySizeGreater): {
      EVALUATE_ANNOTATION(annotation, AnnotationWhenArraySizeGreater,
                          target.is_array() &&
                              target.size() > annotation.value.first,
                          context.instance_location(), annotation.value.second);
    }

    case IS_STEP(AnnotationToParent): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(
          annotation, AnnotationToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location().initial(), annotation.value);
    }

    case IS_STEP(AnnotationBasenameToParent): {
      EVALUATE_ANNOTATION_NO_PRECONDITION(
          annotation, AnnotationBasenameToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location().initial(),
          context.instance_location().back().to_json());
    }

    case IS_STEP(AnnotationLoopPropertiesUnevaluated): {
      EVALUATE_BEGIN(loop, AnnotationLoopPropertiesUnevaluated,
                     target.is_object());
      result = true;
      assert(!loop.value.empty());

      for (const auto &entry : target.as_object()) {
        // TODO: It might be more efficient to get all the annotations we
        // potentially care about as a set first, and the make the loop
        // check for O(1) containment in that set?
        if (context.defines_sibling_annotation(
                loop.value,
                // TODO: This conversion implies a string copy
                sourcemeta::jsontoolkit::JSON{entry.first})) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_annotation_loop_properties_unevaluated_end;
          }
        }

        context.leave();
      }

    evaluate_annotation_loop_properties_unevaluated_end:
      EVALUATE_END(loop, AnnotationLoopPropertiesUnevaluated);
    }

    case IS_STEP(AnnotationLoopItemsUnmarked): {
      EVALUATE_BEGIN(loop, AnnotationLoopItemsUnmarked,
                     target.is_array() &&
                         !context.defines_any_annotation(loop.value));
      // Otherwise you shouldn't be using this step?
      assert(!loop.value.empty());
      const auto &array{target.as_array()};
      result = true;

      for (auto iterator = array.cbegin(); iterator != array.cend();
           ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        context.enter(static_cast<Pointer::Token::Index>(index));
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_annotation_loop_items_unmarked_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_annotation_loop_items_unmarked_end:
      EVALUATE_END(loop, AnnotationLoopItemsUnmarked);
    }

    case IS_STEP(AnnotationLoopItemsUnevaluated): {
      // TODO: This precondition is very expensive due to pointer manipulation
      EVALUATE_BEGIN(
          loop, AnnotationLoopItemsUnevaluated,
          target.is_array() &&
              !context.defines_sibling_annotation(
                  loop.value.mask, sourcemeta::jsontoolkit::JSON{true}));
      const auto &array{target.as_array()};
      result = true;
      auto iterator{array.cbegin()};

      // Determine the proper start based on integer annotations collected for
      // the current instance location by the keyword requested by the user.
      const std::uint64_t start{
          context.largest_annotation_index(loop.value.index)};

      // We need this check, as advancing an iterator past its bounds
      // is considered undefined behavior
      // See https://en.cppreference.com/w/cpp/iterator/advance
      std::advance(iterator,
                   std::min(static_cast<std::ptrdiff_t>(start),
                            static_cast<std::ptrdiff_t>(target.size())));

      for (; iterator != array.cend(); ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};

        // TODO: Can we avoid doing this expensive operation on a loop?
        if (context.defines_sibling_annotation(
                loop.value.filter, sourcemeta::jsontoolkit::JSON{
                                       static_cast<std::size_t>(index)})) {
          continue;
        }

        context.enter(static_cast<Pointer::Token::Index>(index));
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_annotation_loop_items_unevaluated_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_annotation_loop_items_unevaluated_end:
      EVALUATE_END(loop, AnnotationLoopItemsUnevaluated);
    }

    case IS_STEP(AnnotationNot): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, AnnotationNot);
      // Ignore annotations produced inside "not"
      context.mask();
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = true;
          break;
        }
      }

      EVALUATE_END(logical, AnnotationNot);
    }

    case IS_STEP(LogicalNot): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalNot);
      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = true;
          break;
        }
      }

      EVALUATE_END(logical, LogicalNot);
    }

    case IS_STEP(LoopPropertiesMatch): {
      EVALUATE_BEGIN(loop, LoopPropertiesMatch, target.is_object());
      assert(!loop.value.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        const auto index{loop.value.find(entry.first)};
        if (index == loop.value.cend()) {
          continue;
        }

        const auto &substep{loop.children[index->second]};
        assert(std::holds_alternative<LogicalAnd>(substep));
        for (const auto &child : std::get<LogicalAnd>(substep).children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_match_end;
          }
        }
      }

    evaluate_loop_properties_match_end:
      EVALUATE_END(loop, LoopPropertiesMatch);
    }

    case IS_STEP(LoopProperties): {
      EVALUATE_BEGIN(loop, LoopProperties, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_end:
      EVALUATE_END(loop, LoopProperties);
    }

    case IS_STEP(LoopPropertiesRegex): {
      EVALUATE_BEGIN(loop, LoopPropertiesRegex, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (!std::regex_search(entry.first, loop.value.first)) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_regex_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_regex_end:
      EVALUATE_END(loop, LoopPropertiesRegex);
    }

    case IS_STEP(LoopPropertiesExcept): {
      EVALUATE_BEGIN(loop, LoopPropertiesExcept, target.is_object());
      result = true;
      // Otherwise why emit this instruction?
      assert(!loop.value.first.empty() || !loop.value.second.empty());

      for (const auto &entry : target.as_object()) {
        if (std::find(loop.value.first.cbegin(), loop.value.first.cend(),
                      entry.first) != loop.value.first.cend() ||
            std::any_of(loop.value.second.cbegin(), loop.value.second.cend(),
                        [&entry](const auto &pattern) {
                          return std::regex_search(entry.first, pattern.first);
                        })) {
          continue;
        }

        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_except_end;
          }
        }

        context.leave();
      }

    evaluate_loop_properties_except_end:
      EVALUATE_END(loop, LoopPropertiesExcept);
    }

    case IS_STEP(LoopPropertiesType): {
      EVALUATE_BEGIN(loop, LoopPropertiesType, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        const auto &entry_target{context.resolve_target()};
        if (entry_target.type() != loop.value &&
            // In non-strict mode, we consider a real number that represents an
            // integer to be an integer
            (loop.value != sourcemeta::jsontoolkit::JSON::Type::Integer ||
             !entry_target.is_integer_real())) {
          result = false;
          context.leave();
          break;
        }

        context.leave();
      }

      EVALUATE_END(loop, LoopPropertiesType);
    }

    case IS_STEP(LoopPropertiesTypeStrict): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeStrict, target.is_object());
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

      EVALUATE_END(loop, LoopPropertiesTypeStrict);
    }

    case IS_STEP(LoopKeys): {
      EVALUATE_BEGIN(loop, LoopKeys, target.is_object());
      result = true;
      context.target_type(EvaluationContext::TargetType::Key);
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_loop_keys_end;
          }
        }

        context.leave();
      }

    evaluate_loop_keys_end:
      context.target_type(EvaluationContext::TargetType::Value);
      EVALUATE_END(loop, LoopKeys);
    }

    case IS_STEP(LoopItems): {
      EVALUATE_BEGIN(loop, LoopItems, target.is_array());
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
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave();
            goto evaluate_compiler_loop_items_end;
          }
        }

        context.leave();
      }

    evaluate_compiler_loop_items_end:
      EVALUATE_END(loop, LoopItems);
    }

    case IS_STEP(LoopContains): {
      EVALUATE_BEGIN(loop, LoopContains, target.is_array());
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
          if (!evaluate_step(child, callback, context)) {
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

      EVALUATE_END(loop, LoopContains);
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

inline auto
evaluate_internal(sourcemeta::blaze::EvaluationContext &context,
                  const sourcemeta::blaze::Template &steps,
                  const std::optional<sourcemeta::blaze::Callback> &callback)
    -> bool {
  bool overall{true};
  for (const auto &step : steps) {
    if (!evaluate_step(step, callback, context)) {
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

namespace sourcemeta::blaze {

auto evaluate(const Template &steps,
              const sourcemeta::jsontoolkit::JSON &instance,
              const Callback &callback) -> bool {
  EvaluationContext context;
  context.prepare(instance);
  return evaluate_internal(context, steps, callback);
}

auto evaluate(const Template &steps,
              const sourcemeta::jsontoolkit::JSON &instance) -> bool {
  EvaluationContext context;
  context.prepare(instance);
  return evaluate_internal(context, steps, std::nullopt);
}

auto evaluate(const Template &steps, EvaluationContext &context) -> bool {
  return evaluate_internal(context, steps, std::nullopt);
}

} // namespace sourcemeta::blaze
