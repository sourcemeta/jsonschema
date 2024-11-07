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
  const auto track{step_category.track || callback.has_value()};               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track);   \
  const auto &target{context.resolve_target()};                                \
  if (!(precondition)) {                                                       \
    context.pop(step_category.dynamic, track);                                 \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_IF_STRING(step_category, step_type)                     \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  const auto track{step_category.track || callback.has_value()};               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track);   \
  const auto &maybe_target{context.resolve_string_target()};                   \
  if (!maybe_target.has_value()) {                                             \
    context.pop(step_category.dynamic, track);                                 \
    SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                      \
    return true;                                                               \
  }                                                                            \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
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
  const auto track{step_category.track || callback.has_value()};               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track);   \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
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
  const auto track{step_category.track || callback.has_value()};               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track,    \
               std::move(target_check.value()));                               \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(step_category, step_type)               \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  const auto track{step_category.track || callback.has_value()};               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track);   \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(step_category, step_type)   \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     context.instance_location, context.null);                 \
  }                                                                            \
  bool result{true};

#define EVALUATE_BEGIN_PASS_THROUGH(step_category, step_type)                  \
  SOURCEMETA_TRACE_END(trace_dispatch_id, "Dispatch");                         \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  bool result{true};

#define EVALUATE_END(step_category, step_type)                                 \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Post, result, step,                       \
                     context.evaluate_path, context.instance_location,         \
                     context.null);                                            \
  }                                                                            \
  context.pop(step_category.dynamic, track);                                   \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return result;

#define EVALUATE_END_NO_POP(step_category, step_type)                          \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Post, result, step,                       \
                     context.evaluate_path, context.instance_location,         \
                     context.null);                                            \
  }                                                                            \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return result;

#define EVALUATE_END_PASS_THROUGH(step_type)                                   \
  SOURCEMETA_TRACE_END(trace_id, STRINGIFY(step_type));                        \
  return result;

#define EVALUATE_ANNOTATION(step_category, step_type, destination,             \
                            annotation_value)                                  \
  SOURCEMETA_TRACE_START(trace_id, STRINGIFY(step_type));                      \
  const auto &step_category{std::get<step_type>(step)};                        \
  const auto track{step_category.track || callback.has_value()};               \
  assert(track);                                                               \
  context.push(step_category.relative_schema_location,                         \
               step_category.relative_instance_location,                       \
               step_category.schema_resource, step_category.dynamic, track);   \
  if (callback.has_value()) {                                                  \
    callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,   \
                     destination, context.null);                               \
    callback.value()(EvaluationType::Post, true, step, context.evaluate_path,  \
                     destination, annotation_value);                           \
  }                                                                            \
  context.pop(step_category.dynamic, track);                                   \
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

    case IS_STEP(AssertionPropertyTypeEvaluate): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyTypeEvaluate,
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

      if (result) {
        assert(track);
        context.evaluate();
      }

      EVALUATE_END(assertion, AssertionPropertyTypeEvaluate);
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

    case IS_STEP(AssertionPropertyTypeStrictEvaluate): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyTypeStrictEvaluate,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      result = context.resolve_target().type() == assertion.value;

      if (result) {
        assert(track);
        context.evaluate();
      }

      EVALUATE_END(assertion, AssertionPropertyTypeStrictEvaluate);
    }

    case IS_STEP(AssertionPropertyTypeStrictAny): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyTypeStrictAny,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target().type()) !=
                assertion.value.cend());
      EVALUATE_END(assertion, AssertionPropertyTypeStrictAny);
    }

    case IS_STEP(AssertionPropertyTypeStrictAnyEvaluate): {
      EVALUATE_BEGIN_TRY_TARGET(
          assertion, AssertionPropertyTypeStrictAnyEvaluate,
          // Note that here are are referring to the parent
          // object that might hold the given property,
          // before traversing into the actual property
          target.is_object());
      // Now here we refer to the actual property
      result = (std::find(assertion.value.cbegin(), assertion.value.cend(),
                          context.resolve_target().type()) !=
                assertion.value.cend());

      if (result) {
        assert(track);
        context.evaluate();
      }

      EVALUATE_END(assertion, AssertionPropertyTypeStrictAnyEvaluate);
    }

    case IS_STEP(AssertionArrayPrefix): {
      EVALUATE_BEGIN(assertion, AssertionArrayPrefix, target.is_array());
      // Otherwise there is no point in emitting this step
      assert(!assertion.children.empty());
      result = target.empty();
      const auto prefixes{assertion.children.size() - 1};
      const auto array_size{target.size()};
      if (!result) [[likely]] {
        const auto pointer{array_size == prefixes
                               ? prefixes
                               : std::min(array_size, prefixes) - 1};
        const auto &entry{assertion.children[pointer]};
        result = true;
        assert(std::holds_alternative<ControlGroup>(entry));
        for (const auto &child : std::get<ControlGroup>(entry).children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(assertion, AssertionArrayPrefix);
    }

    case IS_STEP(AssertionArrayPrefixEvaluate): {
      EVALUATE_BEGIN(assertion, AssertionArrayPrefixEvaluate,
                     target.is_array());
      // Otherwise there is no point in emitting this step
      assert(!assertion.children.empty());
      result = target.empty();
      assert(track);
      const auto prefixes{assertion.children.size() - 1};
      const auto array_size{target.size()};
      if (!result) [[likely]] {
        const auto pointer{array_size == prefixes
                               ? prefixes
                               : std::min(array_size, prefixes) - 1};
        const auto &entry{assertion.children[pointer]};
        result = true;
        assert(std::holds_alternative<ControlGroup>(entry));
        for (const auto &child : std::get<ControlGroup>(entry).children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            goto evaluate_assertion_array_prefix_evaluate_end;
          }
        }

        assert(result);
        if (array_size == prefixes) {
          context.evaluate();
        } else {
          context.evaluate(0, pointer);
        }
      }

    evaluate_assertion_array_prefix_evaluate_end:
      EVALUATE_END(assertion, AssertionArrayPrefixEvaluate);
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
        if (track) {
          context.evaluate_path.pop_back(
              logical.relative_schema_location.size());

          for (auto cursor = consequence_start; cursor < consequence_end;
               cursor++) {
            if (!evaluate_step(logical.children[cursor], callback, context)) {
              result = false;
              break;
            }
          }

          context.evaluate_path.push_back(logical.relative_schema_location);
        } else {
          for (auto cursor = consequence_start; cursor < consequence_end;
               cursor++) {
            if (!evaluate_step(logical.children[cursor], callback, context)) {
              result = false;
              break;
            }
          }
        }
      }

      EVALUATE_END(logical, LogicalCondition);
    }

    case IS_STEP(ControlGroup): {
      EVALUATE_BEGIN_PASS_THROUGH(control, ControlGroup);
      for (const auto &child : control.children) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END_PASS_THROUGH(ControlGroup);
    }

    case IS_STEP(ControlGroupWhenDefines): {
      EVALUATE_BEGIN_PASS_THROUGH(control, ControlGroupWhenDefines);
      const auto &target{context.resolve_target()};
      assert(!control.children.empty());

      if (target.is_object() && target.defines(control.value)) {
        for (const auto &child : control.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END_PASS_THROUGH(ControlGroupWhenDefines);
    }

    case IS_STEP(ControlLabel): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlLabel);
      assert(!control.children.empty());
      context.labels.try_emplace(control.value, control.children);
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
      EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(control, ControlMark);
      context.labels.try_emplace(control.value, control.children);
      EVALUATE_END_NO_POP(control, ControlMark);
    }

    case IS_STEP(ControlEvaluate): {
      EVALUATE_BEGIN_PASS_THROUGH(control, ControlEvaluate);

      if (callback.has_value()) {
        // TODO: Optimize this case to avoid an extra pointer copy
        auto destination = context.instance_location;
        destination.push_back(control.value);
        callback.value()(EvaluationType::Pre, true, step, context.evaluate_path,
                         destination, context.null);
        context.evaluate(control.value);
        callback.value()(EvaluationType::Post, true, step,
                         context.evaluate_path, destination, context.null);
      } else {
        context.evaluate(control.value);
      }

      EVALUATE_END_PASS_THROUGH(ControlEvaluate);
    }

    case IS_STEP(ControlJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlJump);
      result = true;
      assert(context.labels.contains(control.value));
      for (const auto &child : context.labels.at(control.value).get()) {
        if (!evaluate_step(child, callback, context)) {
          result = false;
          break;
        }
      }

      EVALUATE_END(control, ControlJump);
    }

    case IS_STEP(ControlDynamicAnchorJump): {
      EVALUATE_BEGIN_NO_PRECONDITION(control, ControlDynamicAnchorJump);
      result = false;
      for (const auto &resource : context.resources) {
        const auto label{context.hash(resource, control.value)};
        const auto match{context.labels.find(label)};
        if (match != context.labels.cend()) {
          result = true;
          for (const auto &child : match->second.get()) {
            if (!evaluate_step(child, callback, context)) {
              result = false;
              goto evaluate_control_dynamic_anchor_jump_end;
            }
          }

          break;
        }
      }

    evaluate_control_dynamic_anchor_jump_end:
      EVALUATE_END(control, ControlDynamicAnchorJump);
    }

    case IS_STEP(AnnotationEmit): {
      EVALUATE_ANNOTATION(annotation, AnnotationEmit, context.instance_location,
                          annotation.value);
    }

    case IS_STEP(AnnotationToParent): {
      EVALUATE_ANNOTATION(
          annotation, AnnotationToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location.initial(), annotation.value);
    }

    case IS_STEP(AnnotationBasenameToParent): {
      EVALUATE_ANNOTATION(
          annotation, AnnotationBasenameToParent,
          // TODO: Can we avoid a copy of the instance location here?
          context.instance_location.initial(),
          context.instance_location.back().to_json());
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

    case IS_STEP(LogicalNotEvaluate): {
      EVALUATE_BEGIN_NO_PRECONDITION(logical, LogicalNotEvaluate);

      for (const auto &child : logical.children) {
        if (!evaluate_step(child, callback, context)) {
          result = true;
          break;
        }
      }

      assert(track);
      context.unevaluate();

      EVALUATE_END(logical, LogicalNotEvaluate);
    }

    case IS_STEP(LoopPropertiesUnevaluated): {
      EVALUATE_BEGIN(loop, LoopPropertiesUnevaluated, target.is_object());
      assert(!loop.children.empty());
      assert(track);
      result = true;

      for (const auto &entry : target.as_object()) {
        if (context.is_evaluated({entry.first})) {
          continue;
        }

        context.enter(entry.first, true);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(true);
            // For efficiently breaking from the outer loop too
            goto evaluate_annotation_loop_properties_unevaluated_end;
          }
        }

        context.leave(true);
      }

      // Mark the entire object as evaluated
      context.evaluate();

    evaluate_annotation_loop_properties_unevaluated_end:
      EVALUATE_END(loop, LoopPropertiesUnevaluated);
    }

    case IS_STEP(LoopPropertiesUnevaluatedExcept): {
      EVALUATE_BEGIN(loop, LoopPropertiesUnevaluatedExcept, target.is_object());
      assert(!loop.children.empty());
      assert(track);
      result = true;
      // Otherwise why emit this instruction?
      assert(!std::get<0>(loop.value).empty() ||
             !std::get<1>(loop.value).empty() ||
             !std::get<2>(loop.value).empty());

      for (const auto &entry : target.as_object()) {
        if (std::find(std::get<0>(loop.value).cbegin(),
                      std::get<0>(loop.value).cend(),
                      entry.first) != std::get<0>(loop.value).cend()) {
          continue;
        }

        if (std::any_of(std::get<1>(loop.value).cbegin(),
                        std::get<1>(loop.value).cend(),
                        [&entry](const auto &prefix) {
                          return entry.first.starts_with(prefix);
                        })) {
          continue;
        }

        if (std::any_of(std::get<2>(loop.value).cbegin(),
                        std::get<2>(loop.value).cend(),
                        [&entry](const auto &pattern) {
                          return std::regex_search(entry.first, pattern.first);
                        })) {
          continue;
        }

        if (context.is_evaluated({entry.first})) {
          continue;
        }

        context.enter(entry.first, true);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(true);
            // For efficiently breaking from the outer loop too
            goto evaluate_annotation_loop_properties_unevaluated_except_end;
          }
        }

        context.leave(true);
      }

      // Mark the entire object as evaluated
      context.evaluate();

    evaluate_annotation_loop_properties_unevaluated_except_end:
      EVALUATE_END(loop, LoopPropertiesUnevaluatedExcept);
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
        assert(std::holds_alternative<ControlGroup>(substep));
        for (const auto &child : std::get<ControlGroup>(substep).children) {
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
      assert(!loop.children.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first, track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_end;
          }
        }

        context.leave(track);
      }

    evaluate_loop_properties_end:
      EVALUATE_END(loop, LoopProperties);
    }

    case IS_STEP(LoopPropertiesEvaluate): {
      EVALUATE_BEGIN(loop, LoopPropertiesEvaluate, target.is_object());
      assert(!loop.children.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first, track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_evaluate_end;
          }
        }

        context.leave(track);
      }

      assert(track);
      context.evaluate();

    evaluate_loop_properties_evaluate_end:
      EVALUATE_END(loop, LoopPropertiesEvaluate);
    }

    case IS_STEP(LoopPropertiesRegex): {
      EVALUATE_BEGIN(loop, LoopPropertiesRegex, target.is_object());
      assert(!loop.children.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (!std::regex_search(entry.first, loop.value.first)) {
          continue;
        }

        context.enter(entry.first, track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_regex_end;
          }
        }

        context.leave(track);
      }

    evaluate_loop_properties_regex_end:
      EVALUATE_END(loop, LoopPropertiesRegex);
    }

    case IS_STEP(LoopPropertiesStartsWith): {
      EVALUATE_BEGIN(loop, LoopPropertiesStartsWith, target.is_object());
      assert(!loop.children.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (!entry.first.starts_with(loop.value)) {
          continue;
        }

        context.enter(entry.first, track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_starts_with_end;
          }
        }

        context.leave(track);
      }

    evaluate_loop_properties_starts_with_end:
      EVALUATE_END(loop, LoopPropertiesStartsWith);
    }

    case IS_STEP(LoopPropertiesExcept): {
      EVALUATE_BEGIN(loop, LoopPropertiesExcept, target.is_object());
      assert(!loop.children.empty());
      result = true;
      // Otherwise why emit this instruction?
      assert(!std::get<0>(loop.value).empty() ||
             !std::get<1>(loop.value).empty() ||
             !std::get<2>(loop.value).empty());

      for (const auto &entry : target.as_object()) {
        if (std::find(std::get<0>(loop.value).cbegin(),
                      std::get<0>(loop.value).cend(),
                      entry.first) != std::get<0>(loop.value).cend()) {
          continue;
        }

        if (std::any_of(std::get<1>(loop.value).cbegin(),
                        std::get<1>(loop.value).cend(),
                        [&entry](const auto &prefix) {
                          return entry.first.starts_with(prefix);
                        })) {
          continue;
        }

        if (std::any_of(std::get<2>(loop.value).cbegin(),
                        std::get<2>(loop.value).cend(),
                        [&entry](const auto &pattern) {
                          return std::regex_search(entry.first, pattern.first);
                        })) {
          continue;
        }

        context.enter(entry.first, track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            // For efficiently breaking from the outer loop too
            goto evaluate_loop_properties_except_end;
          }
        }

        context.leave(track);
      }

    evaluate_loop_properties_except_end:
      EVALUATE_END(loop, LoopPropertiesExcept);
    }

    case IS_STEP(LoopPropertiesWhitelist): {
      EVALUATE_BEGIN(loop, LoopPropertiesWhitelist, target.is_object());
      // Otherwise why emit this instruction?
      assert(!loop.value.empty());

      // Otherwise if the number of properties in the instance
      // is larger than the whitelist, then it already violated
      // the whitelist?
      if (target.size() <= loop.value.size()) {
        result = true;
        for (const auto &entry : target.as_object()) {
          if (std::find(loop.value.cbegin(), loop.value.cend(), entry.first) ==
              loop.value.cend()) {
            result = false;
            break;
          }
        }
      }

      EVALUATE_END(loop, LoopPropertiesWhitelist);
    }

    case IS_STEP(LoopPropertiesType): {
      EVALUATE_BEGIN(loop, LoopPropertiesType, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (entry.second.type() != loop.value &&
            // In non-strict mode, we consider a real number that represents an
            // integer to be an integer
            (loop.value != sourcemeta::jsontoolkit::JSON::Type::Integer ||
             !entry.second.is_integer_real())) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopPropertiesType);
    }

    case IS_STEP(LoopPropertiesTypeEvaluate): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeEvaluate, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (entry.second.type() != loop.value &&
            // In non-strict mode, we consider a real number that represents an
            // integer to be an integer
            (loop.value != sourcemeta::jsontoolkit::JSON::Type::Integer ||
             !entry.second.is_integer_real())) {
          result = false;
          goto evaluate_loop_properties_type_evaluate_end;
        }
      }

      assert(track);
      context.evaluate();

    evaluate_loop_properties_type_evaluate_end:
      EVALUATE_END(loop, LoopPropertiesTypeEvaluate);
    }

    case IS_STEP(LoopPropertiesTypeStrict): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeStrict, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (entry.second.type() != loop.value) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopPropertiesTypeStrict);
    }

    case IS_STEP(LoopPropertiesTypeStrictEvaluate): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeStrictEvaluate,
                     target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (entry.second.type() != loop.value) {
          result = false;
          goto evaluate_loop_properties_type_strict_evaluate_end;
        }
      }

      assert(track);
      context.evaluate();

    evaluate_loop_properties_type_strict_evaluate_end:
      EVALUATE_END(loop, LoopPropertiesTypeStrictEvaluate);
    }

    case IS_STEP(LoopPropertiesTypeStrictAny): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeStrictAny, target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (std::find(loop.value.cbegin(), loop.value.cend(),
                      entry.second.type()) == loop.value.cend()) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopPropertiesTypeStrictAny);
    }

    case IS_STEP(LoopPropertiesTypeStrictAnyEvaluate): {
      EVALUATE_BEGIN(loop, LoopPropertiesTypeStrictAnyEvaluate,
                     target.is_object());
      result = true;
      for (const auto &entry : target.as_object()) {
        if (std::find(loop.value.cbegin(), loop.value.cend(),
                      entry.second.type()) == loop.value.cend()) {
          result = false;
          goto evaluate_loop_properties_type_strict_any_evaluate_end;
        }
      }

      assert(track);
      context.evaluate();

    evaluate_loop_properties_type_strict_any_evaluate_end:
      EVALUATE_END(loop, LoopPropertiesTypeStrictAnyEvaluate);
    }

    case IS_STEP(LoopKeys): {
      EVALUATE_BEGIN(loop, LoopKeys, target.is_object());
      assert(!loop.children.empty());
      result = true;
      for (const auto &entry : target.as_object()) {
        context.enter(entry.first, track);
        assert(!context.property_target.has_value());
        context.property_target = std::cref(entry.first);

        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            assert(context.property_target.has_value());
            context.property_target = std::nullopt;
            context.leave(track);
            goto evaluate_loop_keys_end;
          }
        }

        assert(context.property_target.has_value());
        context.property_target = std::nullopt;
        context.leave(track);
      }

    evaluate_loop_keys_end:
      EVALUATE_END(loop, LoopKeys);
    }

    case IS_STEP(LoopItems): {
      EVALUATE_BEGIN(loop, LoopItems,
                     target.is_array() && loop.value < target.size());
      assert(!loop.children.empty());
      const auto &array{target.as_array()};
      result = true;
      auto iterator{array.cbegin()};
      std::advance(iterator, static_cast<std::ptrdiff_t>(loop.value));

      for (; iterator != array.cend(); ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        context.enter(static_cast<Pointer::Token::Index>(index), track);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(track);

            goto evaluate_compiler_loop_items_end;
          }
        }

        context.leave(track);
      }

    evaluate_compiler_loop_items_end:
      EVALUATE_END(loop, LoopItems);
    }

    case IS_STEP(LoopItemsUnevaluated): {
      EVALUATE_BEGIN(loop, LoopItemsUnevaluated, target.is_array());
      assert(!loop.children.empty());
      assert(track);
      const auto &array{target.as_array()};
      result = true;
      for (auto iterator = array.cbegin(); iterator != array.cend();
           ++iterator) {
        const auto index{std::distance(array.cbegin(), iterator)};
        if (context.is_evaluated(
                static_cast<WeakPointer::Token::Index>(index))) {
          continue;
        }

        context.enter(static_cast<Pointer::Token::Index>(index), true);
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            result = false;
            context.leave(true);
            goto evaluate_compiler_annotation_loop_items_unevaluated_end;
          }
        }

        context.leave(true);
      }

      // Mark the entire array as evaluated
      context.evaluate();

    evaluate_compiler_annotation_loop_items_unevaluated_end:
      EVALUATE_END(loop, LoopItemsUnevaluated);
    }

    case IS_STEP(LoopItemsType): {
      EVALUATE_BEGIN(loop, LoopItemsType, target.is_array());
      result = true;
      for (const auto &entry : target.as_array()) {
        if (entry.type() != loop.value &&
            // In non-strict mode, we consider a real number that represents an
            // integer to be an integer
            (loop.value != sourcemeta::jsontoolkit::JSON::Type::Integer ||
             !entry.is_integer_real())) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopItemsType);
    }

    case IS_STEP(LoopItemsTypeStrict): {
      EVALUATE_BEGIN(loop, LoopItemsTypeStrict, target.is_array());
      result = true;
      for (const auto &entry : target.as_array()) {
        if (entry.type() != loop.value) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopItemsTypeStrict);
    }

    case IS_STEP(LoopItemsTypeStrictAny): {
      EVALUATE_BEGIN(loop, LoopItemsTypeStrictAny, target.is_array());
      // Otherwise we are we even emitting this instruction?
      assert(loop.value.size() > 1);
      result = true;
      for (const auto &entry : target.as_array()) {
        if (std::find(loop.value.cbegin(), loop.value.cend(), entry.type()) ==
            loop.value.cend()) {
          result = false;
          break;
        }
      }

      EVALUATE_END(loop, LoopItemsTypeStrictAny);
    }

    case IS_STEP(LoopContains): {
      EVALUATE_BEGIN(loop, LoopContains, target.is_array());
      assert(!loop.children.empty());
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
        context.enter(static_cast<Pointer::Token::Index>(index), track);
        bool subresult{true};
        for (const auto &child : loop.children) {
          if (!evaluate_step(child, callback, context)) {
            subresult = false;
            break;
          }
        }

        context.leave(track);

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
#undef EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH
#undef EVALUATE_BEGIN_PASS_THROUGH
#undef EVALUATE_END
#undef EVALUATE_END_NO_POP
#undef EVALUATE_END_PASS_THROUGH
#undef EVALUATE_ANNOTATION

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
  assert(context.evaluate_path.empty());
  assert(context.evaluate_path_size == 0);
  assert(context.instance_location.empty());
  assert(context.resources.empty());
  // We should end up at the root of the instance
  assert(context.instances.size() == 1);
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
