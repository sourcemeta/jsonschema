#ifndef SOURCEMETA_BLAZE_EVALUATOR_TEMPLATE_H
#define SOURCEMETA_BLAZE_EVALUATOR_TEMPLATE_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/blaze/evaluator_value.h>

#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>     // std::uint8_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::blaze {

// For fast internal instruction dispatching. It must stay
// in sync with the variant ordering above
/// @ingroup evaluator
enum class InstructionIndex : std::uint8_t {
  AssertionFail = 0,
  AssertionDefines,
  AssertionDefinesStrict,
  AssertionDefinesAll,
  AssertionDefinesAllStrict,
  AssertionDefinesExactly,
  AssertionDefinesExactlyStrict,
  AssertionDefinesExactlyStrictHash3,
  AssertionPropertyDependencies,
  AssertionType,
  AssertionTypeAny,
  AssertionTypeStrict,
  AssertionTypeStrictAny,
  AssertionNotTypeStrictAny,
  AssertionTypeStringBounded,
  AssertionTypeStringUpper,
  AssertionTypeArrayBounded,
  AssertionTypeArrayUpper,
  AssertionTypeObjectBounded,
  AssertionTypeObjectUpper,
  AssertionRegex,
  AssertionStringSizeLess,
  AssertionStringSizeGreater,
  AssertionArraySizeLess,
  AssertionArraySizeGreater,
  AssertionObjectSizeLess,
  AssertionObjectSizeGreater,
  AssertionEqual,
  AssertionEqualsAny,
  AssertionEqualsAnyStringHash,
  AssertionGreaterEqual,
  AssertionLessEqual,
  AssertionGreater,
  AssertionLess,
  AssertionUnique,
  AssertionDivisible,
  AssertionTypeIntegerBounded,
  AssertionTypeIntegerBoundedStrict,
  AssertionTypeIntegerLowerBound,
  AssertionTypeIntegerLowerBoundStrict,
  AssertionNumberIntegerBounded,
  AssertionNumberType,
  AssertionStringType,
  AssertionPropertyType,
  AssertionPropertyTypeEvaluate,
  AssertionPropertyTypeStrict,
  AssertionPropertyTypeStrictEvaluate,
  AssertionPropertyTypeStrictAny,
  AssertionPropertyTypeStrictAnyEvaluate,
  AssertionPropertyEqual,
  AssertionPropertyEqualsAnyStringHash,
  AssertionArrayPrefix,
  AssertionArrayPrefixEvaluate,
  AssertionObjectPropertiesSimple,
  AnnotationEmit,
  AnnotationEmitWrapped,
  AnnotationToParent,
  AnnotationBasenameToParent,
  Evaluate,
  LogicalNot,
  LogicalNotEvaluate,
  LogicalOr,
  LogicalOrEvaluate,
  LogicalAnd,
  LogicalXor,
  LogicalXorEvaluate,
  LogicalCondition,
  LogicalConditionEvaluate,
  LogicalWhenType,
  LogicalWhenDefines,
  LogicalWhenArraySizeGreater,
  LoopPropertiesUnevaluated,
  LoopPropertiesUnevaluatedExcept,
  LoopPropertiesMatch,
  LoopPropertiesMatchClosed,
  LoopProperties,
  LoopPropertiesEvaluate,
  LoopPropertiesRegex,
  LoopPropertiesRegexClosed,
  LoopPropertiesStartsWith,
  LoopPropertiesExcept,
  LoopPropertiesType,
  LoopPropertiesTypeEvaluate,
  LoopPropertiesExactlyTypeStrict,
  LoopPropertiesExactlyTypeStrictHash,
  LoopPropertiesTypeStrict,
  LoopPropertiesTypeStrictEvaluate,
  LoopPropertiesTypeStrictAny,
  LoopPropertiesTypeStrictAnyEvaluate,
  LoopKeys,
  LoopItems,
  LoopItemsFrom,
  LoopItemsUnevaluated,
  LoopItemsType,
  LoopItemsTypeStrict,
  LoopItemsTypeStrictAny,
  LoopItemsPropertiesExactlyTypeStrictHash,
  LoopItemsPropertiesExactlyTypeStrictHash3,
  LoopItemsIntegerBounded,
  LoopItemsIntegerBoundedSized,
  LoopItemsArrayBoundedTypeStrictAny,
  LoopContains,
  ControlGroup,
  ControlGroupWhenDefines,
  ControlGroupWhenDefinesDirect,
  ControlGroupWhenType,
  ControlEvaluate,
  ControlFrame,
  ControlResource,
  ControlDynamicAnchorJump,
  ControlJump,
  ControlResourceJump
};

/// @ingroup evaluator
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
constexpr std::string_view INSTRUCTION_NAMES[] = {
    "AssertionFail",
    "AssertionDefines",
    "AssertionDefinesStrict",
    "AssertionDefinesAll",
    "AssertionDefinesAllStrict",
    "AssertionDefinesExactly",
    "AssertionDefinesExactlyStrict",
    "AssertionDefinesExactlyStrictHash3",
    "AssertionPropertyDependencies",
    "AssertionType",
    "AssertionTypeAny",
    "AssertionTypeStrict",
    "AssertionTypeStrictAny",
    "AssertionNotTypeStrictAny",
    "AssertionTypeStringBounded",
    "AssertionTypeStringUpper",
    "AssertionTypeArrayBounded",
    "AssertionTypeArrayUpper",
    "AssertionTypeObjectBounded",
    "AssertionTypeObjectUpper",
    "AssertionRegex",
    "AssertionStringSizeLess",
    "AssertionStringSizeGreater",
    "AssertionArraySizeLess",
    "AssertionArraySizeGreater",
    "AssertionObjectSizeLess",
    "AssertionObjectSizeGreater",
    "AssertionEqual",
    "AssertionEqualsAny",
    "AssertionEqualsAnyStringHash",
    "AssertionGreaterEqual",
    "AssertionLessEqual",
    "AssertionGreater",
    "AssertionLess",
    "AssertionUnique",
    "AssertionDivisible",
    "AssertionTypeIntegerBounded",
    "AssertionTypeIntegerBoundedStrict",
    "AssertionTypeIntegerLowerBound",
    "AssertionTypeIntegerLowerBoundStrict",
    "AssertionNumberIntegerBounded",
    "AssertionNumberType",
    "AssertionStringType",
    "AssertionPropertyType",
    "AssertionPropertyTypeEvaluate",
    "AssertionPropertyTypeStrict",
    "AssertionPropertyTypeStrictEvaluate",
    "AssertionPropertyTypeStrictAny",
    "AssertionPropertyTypeStrictAnyEvaluate",
    "AssertionPropertyEqual",
    "AssertionPropertyEqualsAnyStringHash",
    "AssertionArrayPrefix",
    "AssertionArrayPrefixEvaluate",
    "AssertionObjectPropertiesSimple",
    "AnnotationEmit",
    "AnnotationEmitWrapped",
    "AnnotationToParent",
    "AnnotationBasenameToParent",
    "Evaluate",
    "LogicalNot",
    "LogicalNotEvaluate",
    "LogicalOr",
    "LogicalOrEvaluate",
    "LogicalAnd",
    "LogicalXor",
    "LogicalXorEvaluate",
    "LogicalCondition",
    "LogicalConditionEvaluate",
    "LogicalWhenType",
    "LogicalWhenDefines",
    "LogicalWhenArraySizeGreater",
    "LoopPropertiesUnevaluated",
    "LoopPropertiesUnevaluatedExcept",
    "LoopPropertiesMatch",
    "LoopPropertiesMatchClosed",
    "LoopProperties",
    "LoopPropertiesEvaluate",
    "LoopPropertiesRegex",
    "LoopPropertiesRegexClosed",
    "LoopPropertiesStartsWith",
    "LoopPropertiesExcept",
    "LoopPropertiesType",
    "LoopPropertiesTypeEvaluate",
    "LoopPropertiesExactlyTypeStrict",
    "LoopPropertiesExactlyTypeStrictHash",
    "LoopPropertiesTypeStrict",
    "LoopPropertiesTypeStrictEvaluate",
    "LoopPropertiesTypeStrictAny",
    "LoopPropertiesTypeStrictAnyEvaluate",
    "LoopKeys",
    "LoopItems",
    "LoopItemsFrom",
    "LoopItemsUnevaluated",
    "LoopItemsType",
    "LoopItemsTypeStrict",
    "LoopItemsTypeStrictAny",
    "LoopItemsPropertiesExactlyTypeStrictHash",
    "LoopItemsPropertiesExactlyTypeStrictHash3",
    "LoopItemsIntegerBounded",
    "LoopItemsIntegerBoundedSized",
    "LoopItemsArrayBoundedTypeStrictAny",
    "LoopContains",
    "ControlGroup",
    "ControlGroupWhenDefines",
    "ControlGroupWhenDefinesDirect",
    "ControlGroupWhenType",
    "ControlEvaluate",
    "ControlFrame",
    "ControlResource",
    "ControlDynamicAnchorJump",
    "ControlJump",
    "ControlResourceJump"};

/// @ingroup evaluator
/// Check if a given instruction type corresponds to an annotation
inline auto is_annotation(const InstructionIndex type) noexcept -> bool {
  switch (type) {
    // NOLINTNEXTLINE(bugprone-branch-clone)
    case InstructionIndex::AnnotationBasenameToParent:
      return true;
    case InstructionIndex::AnnotationToParent:
      return true;
    case InstructionIndex::AnnotationEmit:
      return true;
    case InstructionIndex::AnnotationEmitWrapped:
      return true;
    default:
      return false;
  }
}

/// @ingroup evaluator
/// Check if a given instruction type holds alternative branches, where a
/// branch that does not hold contributes nothing to the overall result
inline auto is_disjunction(const InstructionIndex type) noexcept -> bool {
  switch (type) {
    case InstructionIndex::LogicalOr:
    case InstructionIndex::LogicalOrEvaluate:
    case InstructionIndex::LogicalXor:
    case InstructionIndex::LogicalXorEvaluate:
      return true;
    default:
      return false;
  }
}

/// @ingroup evaluator
/// Check if a given instruction type holds a condition followed by its
/// mutually exclusive consequence and alternative
inline auto is_conditional(const InstructionIndex type) noexcept -> bool {
  switch (type) {
    case InstructionIndex::LogicalCondition:
    case InstructionIndex::LogicalConditionEvaluate:
      return true;
    default:
      return false;
  }
}

/// @ingroup evaluator
/// Check if a given instruction type steers evaluation rather than asserting
/// on the instance, which every one of them does from a location of its own
inline auto is_control(const InstructionIndex type) noexcept -> bool {
  switch (type) {
    case InstructionIndex::ControlGroup:
    case InstructionIndex::ControlGroupWhenDefines:
    case InstructionIndex::ControlGroupWhenDefinesDirect:
    case InstructionIndex::ControlGroupWhenType:
    case InstructionIndex::ControlEvaluate:
    case InstructionIndex::ControlFrame:
    case InstructionIndex::ControlResource:
    case InstructionIndex::ControlDynamicAnchorJump:
    case InstructionIndex::ControlJump:
    case InstructionIndex::ControlResourceJump:
      return true;
    default:
      return false;
  }
}

/// @ingroup evaluator
/// Check if a given instruction type corresponds to a wrapped annotation
inline auto is_wrapped_annotation(const InstructionIndex type) noexcept
    -> bool {
  switch (type) {
    case InstructionIndex::AnnotationBasenameToParent:
    case InstructionIndex::AnnotationEmitWrapped:
      return true;
    default:
      return false;
  }
}

// Forward declaration for defining a circular structure
#ifndef DOXYGEN
struct Instruction;
#endif

// Forward declaration, as the template is defined in terms of this header
#ifndef DOXYGEN
struct Template;
#endif

/// @ingroup evaluator
/// Represents a set of schema compilation steps that can be evaluated
using Instructions = std::vector<Instruction>;

/// @ingroup evaluator
/// Satellite data for an instruction that is not needed during fast evaluation
struct InstructionExtra {
  sourcemeta::core::Pointer relative_schema_location;
  std::string keyword_location;
  std::size_t schema_resource;
  std::size_t vocabulary;
  /// A one-based index into Template::messages, where zero means the
  /// instruction has no message template of its own
  std::size_t message;
  /// The template to render instead when the instruction has something of its
  /// own to report, such as which properties were missing. Zero when the
  /// instruction says the same thing either way
  std::size_t message_detail;
};

/// @ingroup evaluator
/// Represents a single instruction to be evaluated
// NOLINTNEXTLINE(bugprone-exception-escape)
struct Instruction {
  InstructionIndex type;
  sourcemeta::core::Pointer relative_instance_location;
  Value value;
  Instructions children;
  std::size_t extra_index;
};

/// @ingroup evaluator
///
/// This function translates a "post" step execution into a human-readable
/// string. Useful as the building block for producing user-friendly evaluation
/// results.
///
/// Note that describing a "pre" step execution is NOT supported.
auto SOURCEMETA_BLAZE_EVALUATOR_EXPORT describe(
    const Template &schema_template, const bool valid, const Instruction &step,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &instance,
    const sourcemeta::core::JSON &annotation) -> std::string;

} // namespace sourcemeta::blaze

#endif
