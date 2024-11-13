#ifndef SOURCEMETA_BLAZE_EVALUATOR_TEMPLATE_H
#define SOURCEMETA_BLAZE_EVALUATOR_TEMPLATE_H

#include <sourcemeta/blaze/evaluator_value.h>

#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <cstdint> // std::uint8_t
#include <string>  // std::string
#include <vector>  // std::vector

namespace sourcemeta::blaze {

// Forward declarations for the sole purpose of being bale to define circular
// structures
#ifndef DOXYGEN
struct AssertionFail;
struct AssertionDefines;
struct AssertionDefinesAll;
struct AssertionPropertyDependencies;
struct AssertionType;
struct AssertionTypeAny;
struct AssertionTypeStrict;
struct AssertionTypeStrictAny;
struct AssertionTypeStringBounded;
struct AssertionTypeArrayBounded;
struct AssertionTypeObjectBounded;
struct AssertionRegex;
struct AssertionStringSizeLess;
struct AssertionStringSizeGreater;
struct AssertionArraySizeLess;
struct AssertionArraySizeGreater;
struct AssertionObjectSizeLess;
struct AssertionObjectSizeGreater;
struct AssertionEqual;
struct AssertionEqualsAny;
struct AssertionGreaterEqual;
struct AssertionLessEqual;
struct AssertionGreater;
struct AssertionLess;
struct AssertionUnique;
struct AssertionDivisible;
struct AssertionStringType;
struct AssertionPropertyType;
struct AssertionPropertyTypeEvaluate;
struct AssertionPropertyTypeStrict;
struct AssertionPropertyTypeStrictEvaluate;
struct AssertionPropertyTypeStrictAny;
struct AssertionPropertyTypeStrictAnyEvaluate;
struct AssertionArrayPrefix;
struct AssertionArrayPrefixEvaluate;
struct AnnotationEmit;
struct AnnotationToParent;
struct AnnotationBasenameToParent;
struct LogicalNot;
struct LogicalNotEvaluate;
struct LogicalOr;
struct LogicalAnd;
struct LogicalXor;
struct LogicalCondition;
struct LogicalWhenType;
struct LogicalWhenDefines;
struct LogicalWhenArraySizeGreater;
struct LoopPropertiesUnevaluated;
struct LoopPropertiesUnevaluatedExcept;
struct LoopPropertiesMatch;
struct LoopProperties;
struct LoopPropertiesEvaluate;
struct LoopPropertiesRegex;
struct LoopPropertiesStartsWith;
struct LoopPropertiesExcept;
struct LoopPropertiesWhitelist;
struct LoopPropertiesType;
struct LoopPropertiesTypeEvaluate;
struct LoopPropertiesTypeStrict;
struct LoopPropertiesTypeStrictEvaluate;
struct LoopPropertiesTypeStrictAny;
struct LoopPropertiesTypeStrictAnyEvaluate;
struct LoopKeys;
struct LoopItems;
struct LoopItemsUnevaluated;
struct LoopItemsType;
struct LoopItemsTypeStrict;
struct LoopItemsTypeStrictAny;
struct LoopContains;
struct ControlGroup;
struct ControlGroupWhenDefines;
struct ControlLabel;
struct ControlMark;
struct ControlEvaluate;
struct ControlJump;
struct ControlDynamicAnchorJump;
#endif

/// @ingroup evaluator
/// Represents a schema compilation step that can be evaluated
using Template = std::vector<std::variant<
    AssertionFail, AssertionDefines, AssertionDefinesAll,
    AssertionPropertyDependencies, AssertionType, AssertionTypeAny,
    AssertionTypeStrict, AssertionTypeStrictAny, AssertionTypeStringBounded,
    AssertionTypeArrayBounded, AssertionTypeObjectBounded, AssertionRegex,
    AssertionStringSizeLess, AssertionStringSizeGreater, AssertionArraySizeLess,
    AssertionArraySizeGreater, AssertionObjectSizeLess,
    AssertionObjectSizeGreater, AssertionEqual, AssertionEqualsAny,
    AssertionGreaterEqual, AssertionLessEqual, AssertionGreater, AssertionLess,
    AssertionUnique, AssertionDivisible, AssertionStringType,
    AssertionPropertyType, AssertionPropertyTypeEvaluate,
    AssertionPropertyTypeStrict, AssertionPropertyTypeStrictEvaluate,
    AssertionPropertyTypeStrictAny, AssertionPropertyTypeStrictAnyEvaluate,
    AssertionArrayPrefix, AssertionArrayPrefixEvaluate, AnnotationEmit,
    AnnotationToParent, AnnotationBasenameToParent, LogicalNot,
    LogicalNotEvaluate, LogicalOr, LogicalAnd, LogicalXor, LogicalCondition,
    LogicalWhenType, LogicalWhenDefines, LogicalWhenArraySizeGreater,
    LoopPropertiesUnevaluated, LoopPropertiesUnevaluatedExcept,
    LoopPropertiesMatch, LoopProperties, LoopPropertiesEvaluate,
    LoopPropertiesRegex, LoopPropertiesStartsWith, LoopPropertiesExcept,
    LoopPropertiesWhitelist, LoopPropertiesType, LoopPropertiesTypeEvaluate,
    LoopPropertiesTypeStrict, LoopPropertiesTypeStrictEvaluate,
    LoopPropertiesTypeStrictAny, LoopPropertiesTypeStrictAnyEvaluate, LoopKeys,
    LoopItems, LoopItemsUnevaluated, LoopItemsType, LoopItemsTypeStrict,
    LoopItemsTypeStrictAny, LoopContains, ControlGroup, ControlGroupWhenDefines,
    ControlLabel, ControlMark, ControlEvaluate, ControlJump,
    ControlDynamicAnchorJump>>;

#if !defined(DOXYGEN)
// For fast internal instruction dispatching. It must stay
// in sync with the variant ordering above
enum class TemplateIndex : std::uint8_t {
  AssertionFail = 0,
  AssertionDefines,
  AssertionDefinesAll,
  AssertionPropertyDependencies,
  AssertionType,
  AssertionTypeAny,
  AssertionTypeStrict,
  AssertionTypeStrictAny,
  AssertionTypeStringBounded,
  AssertionTypeArrayBounded,
  AssertionTypeObjectBounded,
  AssertionRegex,
  AssertionStringSizeLess,
  AssertionStringSizeGreater,
  AssertionArraySizeLess,
  AssertionArraySizeGreater,
  AssertionObjectSizeLess,
  AssertionObjectSizeGreater,
  AssertionEqual,
  AssertionEqualsAny,
  AssertionGreaterEqual,
  AssertionLessEqual,
  AssertionGreater,
  AssertionLess,
  AssertionUnique,
  AssertionDivisible,
  AssertionStringType,
  AssertionPropertyType,
  AssertionPropertyTypeEvaluate,
  AssertionPropertyTypeStrict,
  AssertionPropertyTypeStrictEvaluate,
  AssertionPropertyTypeStrictAny,
  AssertionPropertyTypeStrictAnyEvaluate,
  AssertionArrayPrefix,
  AssertionArrayPrefixEvaluate,
  AnnotationEmit,
  AnnotationToParent,
  AnnotationBasenameToParent,
  LogicalNot,
  LogicalNotEvaluate,
  LogicalOr,
  LogicalAnd,
  LogicalXor,
  LogicalCondition,
  LogicalWhenType,
  LogicalWhenDefines,
  LogicalWhenArraySizeGreater,
  LoopPropertiesUnevaluated,
  LoopPropertiesUnevaluatedExcept,
  LoopPropertiesMatch,
  LoopProperties,
  LoopPropertiesEvaluate,
  LoopPropertiesRegex,
  LoopPropertiesStartsWith,
  LoopPropertiesExcept,
  LoopPropertiesWhitelist,
  LoopPropertiesType,
  LoopPropertiesTypeEvaluate,
  LoopPropertiesTypeStrict,
  LoopPropertiesTypeStrictEvaluate,
  LoopPropertiesTypeStrictAny,
  LoopPropertiesTypeStrictAnyEvaluate,
  LoopKeys,
  LoopItems,
  LoopItemsUnevaluated,
  LoopItemsType,
  LoopItemsTypeStrict,
  LoopItemsTypeStrictAny,
  LoopContains,
  ControlGroup,
  ControlGroupWhenDefines,
  ControlLabel,
  ControlMark,
  ControlEvaluate,
  ControlJump,
  ControlDynamicAnchorJump
};
#endif

#define DEFINE_STEP_WITH_VALUE(category, name, type)                           \
  struct category##name {                                                      \
    const sourcemeta::jsontoolkit::Pointer relative_schema_location;           \
    const sourcemeta::jsontoolkit::Pointer relative_instance_location;         \
    const std::string keyword_location;                                        \
    const std::size_t schema_resource;                                         \
    const bool dynamic;                                                        \
    const bool track;                                                          \
    const type value;                                                          \
  };

#define DEFINE_STEP_APPLICATOR(category, name, type)                           \
  struct category##name {                                                      \
    const sourcemeta::jsontoolkit::Pointer relative_schema_location;           \
    const sourcemeta::jsontoolkit::Pointer relative_instance_location;         \
    const std::string keyword_location;                                        \
    const std::size_t schema_resource;                                         \
    const bool dynamic;                                                        \
    const bool track;                                                          \
    const type value;                                                          \
    const Template children;                                                   \
  };

/// @defgroup evaluator_instructions Instruction Set
/// @ingroup evaluator
/// @brief The set of instructions supported by the evaluator.
/// @details
///
/// Every instruction operates at a specific instance location and with the
/// given value, whose type depends on the instruction.

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that always fails
DEFINE_STEP_WITH_VALUE(Assertion, Fail, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a given property
DEFINE_STEP_WITH_VALUE(Assertion, Defines, ValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties
DEFINE_STEP_WITH_VALUE(Assertion, DefinesAll, ValueStrings)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties if it defines other set of properties
DEFINE_STEP_WITH_VALUE(Assertion, PropertyDependencies, ValueStringMap)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type
DEFINE_STEP_WITH_VALUE(Assertion, Type, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types
DEFINE_STEP_WITH_VALUE(Assertion, TypeAny, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrict, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrictAny, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type string and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeStringBounded, ValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type array and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeArrayBounded, ValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type object and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeObjectBounded, ValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a string against an
/// ECMA regular expression
DEFINE_STEP_WITH_VALUE(Assertion, Regex, ValueRegex)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// less than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeLess, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// greater than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeGreater, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// less than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeLess, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// greater than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeGreater, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// less than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeLess, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// greater than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeGreater, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks the instance equals
/// a given JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Equal, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that a JSON document
/// is equal to at least one of the given elements
DEFINE_STEP_WITH_VALUE(Assertion, EqualsAny, ValueArray)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, GreaterEqual, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, LessEqual, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Greater, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Less, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given JSON array
/// does not contain duplicate items
DEFINE_STEP_WITH_VALUE(Assertion, Unique, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a number is
/// divisible by another number
DEFINE_STEP_WITH_VALUE(Assertion, Divisible, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that a string is of
/// a certain type
DEFINE_STEP_WITH_VALUE(Assertion, StringType, ValueStringType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present
DEFINE_STEP_WITH_VALUE(Assertion, PropertyType, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present and marks evaluation
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeEvaluate, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present (strict mode)
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrict, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present (strict mode) and marks evaluation
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrictEvaluate, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given set of types if present (strict mode)
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrictAny, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given set of types if present (strict mode) and marks
/// evaluation
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrictAnyEvaluate, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that applies substeps to the
/// beginning of an array
DEFINE_STEP_APPLICATOR(Assertion, ArrayPrefix, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that applies substeps to the
/// beginning of an array and marks evaluation
DEFINE_STEP_APPLICATOR(Assertion, ArrayPrefixEvaluate, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation
DEFINE_STEP_WITH_VALUE(Annotation, Emit, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, ToParent, ValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits the current basename as an
/// annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, BasenameToParent, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a negation
DEFINE_STEP_APPLICATOR(Logical, Not, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a negation and
/// discards evaluation marks after its execution
DEFINE_STEP_APPLICATOR(Logical, NotEvaluate, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a disjunction
DEFINE_STEP_APPLICATOR(Logical, Or, ValueBoolean)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction
DEFINE_STEP_APPLICATOR(Logical, And, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents an exclusive
/// disjunction
DEFINE_STEP_APPLICATOR(Logical, Xor, ValueBoolean)

/// @ingroup evaluator_instructions
/// @brief Represents an imperative conditional compiler logical step
DEFINE_STEP_APPLICATOR(Logical, Condition, ValueIndexPair)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is of a given type
DEFINE_STEP_APPLICATOR(Logical, WhenType, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is an object and defines a given property
DEFINE_STEP_APPLICATOR(Logical, WhenDefines, ValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the array instance size is greater than the given number
DEFINE_STEP_APPLICATOR(Logical, WhenArraySizeGreater, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// were not previously evaluated
DEFINE_STEP_APPLICATOR(Loop, PropertiesUnevaluated, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties are not
/// in the given blacklist and were not previously evaluated
DEFINE_STEP_APPLICATOR(Loop, PropertiesUnevaluatedExcept, ValuePropertyFilter)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that matches steps to object properties
DEFINE_STEP_APPLICATOR(Loop, PropertiesMatch, ValueNamedIndexes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties
DEFINE_STEP_APPLICATOR(Loop, Properties, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties and
/// marks them as evaluated
DEFINE_STEP_APPLICATOR(Loop, PropertiesEvaluate, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// match a given ECMA regular expression
DEFINE_STEP_APPLICATOR(Loop, PropertiesRegex, ValueRegex)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// start with a given string
DEFINE_STEP_APPLICATOR(Loop, PropertiesStartsWith, ValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// do not match the given property filters
DEFINE_STEP_APPLICATOR(Loop, PropertiesExcept, ValuePropertyFilter)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that fails on properties that are not part
/// of the given whitelist
DEFINE_STEP_WITH_VALUE(Loop, PropertiesWhitelist, ValueStrings)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type
DEFINE_STEP_WITH_VALUE(Loop, PropertiesType, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type and marks evaluation
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeEvaluate, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type (strict mode)
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeStrict, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type (strict mode) and marks evaluation
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeStrictEvaluate, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// set of given types (strict mode)
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeStrictAny, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// set of given types (strict mode) and marks evaluation
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeStrictAnyEvaluate, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object property keys
DEFINE_STEP_APPLICATOR(Loop, Keys, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items starting from
/// a given index
DEFINE_STEP_APPLICATOR(Loop, Items, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over unevaluated array items
DEFINE_STEP_APPLICATOR(Loop, ItemsUnevaluated, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items checking their
/// type
DEFINE_STEP_WITH_VALUE(Loop, ItemsType, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items checking their
/// type (strict mode)
DEFINE_STEP_WITH_VALUE(Loop, ItemsTypeStrict, ValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items checking their
/// type (strict mode)
DEFINE_STEP_WITH_VALUE(Loop, ItemsTypeStrictAny, ValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks array items match a given
/// criteria
DEFINE_STEP_APPLICATOR(Loop, Contains, ValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that groups a set of steps, but is not
/// evaluated on its own
DEFINE_STEP_APPLICATOR(Control, Group, ValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that groups a set of steps if a given
/// object property exists, but is not evaluated on its own
DEFINE_STEP_APPLICATOR(Control, GroupWhenDefines, ValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of a mark to jump to while
/// executing children instructions
DEFINE_STEP_APPLICATOR(Control, Label, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of a mark to jump to, but
/// without executing children instructions
DEFINE_STEP_APPLICATOR(Control, Mark, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that marks the current instance location
/// as evaluated
DEFINE_STEP_WITH_VALUE(Control, Evaluate, ValuePointer)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of jumping into a
/// pre-registered label
DEFINE_STEP_WITH_VALUE(Control, Jump, ValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of jump to a dynamic anchor
DEFINE_STEP_WITH_VALUE(Control, DynamicAnchorJump, ValueString)

#undef DEFINE_STEP_WITH_VALUE
#undef DEFINE_STEP_APPLICATOR

} // namespace sourcemeta::blaze

#endif
