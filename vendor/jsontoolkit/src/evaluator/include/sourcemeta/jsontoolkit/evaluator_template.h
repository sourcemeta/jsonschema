#ifndef SOURCEMETA_JSONTOOLKIT_EVALUATOR_TEMPLATE_H
#define SOURCEMETA_JSONTOOLKIT_EVALUATOR_TEMPLATE_H

#include <sourcemeta/jsontoolkit/evaluator_value.h>

#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <cstdint> // std::uint8_t
#include <string>  // std::string
#include <vector>  // std::vector

namespace sourcemeta::jsontoolkit {

// Forward declarations for the sole purpose of being bale to define circular
// structures
#ifndef DOXYGEN
struct SchemaCompilerAssertionFail;
struct SchemaCompilerAssertionDefines;
struct SchemaCompilerAssertionDefinesAll;
struct SchemaCompilerAssertionPropertyDependencies;
struct SchemaCompilerAssertionType;
struct SchemaCompilerAssertionTypeAny;
struct SchemaCompilerAssertionTypeStrict;
struct SchemaCompilerAssertionTypeStrictAny;
struct SchemaCompilerAssertionTypeStringBounded;
struct SchemaCompilerAssertionTypeArrayBounded;
struct SchemaCompilerAssertionTypeObjectBounded;
struct SchemaCompilerAssertionRegex;
struct SchemaCompilerAssertionStringSizeLess;
struct SchemaCompilerAssertionStringSizeGreater;
struct SchemaCompilerAssertionArraySizeLess;
struct SchemaCompilerAssertionArraySizeGreater;
struct SchemaCompilerAssertionObjectSizeLess;
struct SchemaCompilerAssertionObjectSizeGreater;
struct SchemaCompilerAssertionEqual;
struct SchemaCompilerAssertionEqualsAny;
struct SchemaCompilerAssertionGreaterEqual;
struct SchemaCompilerAssertionLessEqual;
struct SchemaCompilerAssertionGreater;
struct SchemaCompilerAssertionLess;
struct SchemaCompilerAssertionUnique;
struct SchemaCompilerAssertionDivisible;
struct SchemaCompilerAssertionStringType;
struct SchemaCompilerAssertionPropertyType;
struct SchemaCompilerAssertionPropertyTypeStrict;
struct SchemaCompilerAnnotationEmit;
struct SchemaCompilerAnnotationWhenArraySizeEqual;
struct SchemaCompilerAnnotationWhenArraySizeGreater;
struct SchemaCompilerAnnotationToParent;
struct SchemaCompilerAnnotationBasenameToParent;
struct SchemaCompilerLogicalOr;
struct SchemaCompilerLogicalAnd;
struct SchemaCompilerLogicalXor;
struct SchemaCompilerLogicalCondition;
struct SchemaCompilerLogicalNot;
struct SchemaCompilerLogicalWhenType;
struct SchemaCompilerLogicalWhenDefines;
struct SchemaCompilerLogicalWhenArraySizeGreater;
struct SchemaCompilerLogicalWhenArraySizeEqual;
struct SchemaCompilerLoopPropertiesMatch;
struct SchemaCompilerLoopProperties;
struct SchemaCompilerLoopPropertiesRegex;
struct SchemaCompilerLoopPropertiesNoAnnotation;
struct SchemaCompilerLoopPropertiesExcept;
struct SchemaCompilerLoopPropertiesType;
struct SchemaCompilerLoopPropertiesTypeStrict;
struct SchemaCompilerLoopKeys;
struct SchemaCompilerLoopItems;
struct SchemaCompilerLoopItemsUnmarked;
struct SchemaCompilerLoopItemsUnevaluated;
struct SchemaCompilerLoopContains;
struct SchemaCompilerControlLabel;
struct SchemaCompilerControlMark;
struct SchemaCompilerControlJump;
struct SchemaCompilerControlDynamicAnchorJump;
#endif

/// @ingroup evaluator
/// Represents a schema compilation step that can be evaluated
using SchemaCompilerTemplate = std::vector<std::variant<
    SchemaCompilerAssertionFail, SchemaCompilerAssertionDefines,
    SchemaCompilerAssertionDefinesAll,
    SchemaCompilerAssertionPropertyDependencies, SchemaCompilerAssertionType,
    SchemaCompilerAssertionTypeAny, SchemaCompilerAssertionTypeStrict,
    SchemaCompilerAssertionTypeStrictAny,
    SchemaCompilerAssertionTypeStringBounded,
    SchemaCompilerAssertionTypeArrayBounded,
    SchemaCompilerAssertionTypeObjectBounded, SchemaCompilerAssertionRegex,
    SchemaCompilerAssertionStringSizeLess,
    SchemaCompilerAssertionStringSizeGreater,
    SchemaCompilerAssertionArraySizeLess,
    SchemaCompilerAssertionArraySizeGreater,
    SchemaCompilerAssertionObjectSizeLess,
    SchemaCompilerAssertionObjectSizeGreater, SchemaCompilerAssertionEqual,
    SchemaCompilerAssertionEqualsAny, SchemaCompilerAssertionGreaterEqual,
    SchemaCompilerAssertionLessEqual, SchemaCompilerAssertionGreater,
    SchemaCompilerAssertionLess, SchemaCompilerAssertionUnique,
    SchemaCompilerAssertionDivisible, SchemaCompilerAssertionStringType,
    SchemaCompilerAssertionPropertyType,
    SchemaCompilerAssertionPropertyTypeStrict, SchemaCompilerAnnotationEmit,
    SchemaCompilerAnnotationWhenArraySizeEqual,
    SchemaCompilerAnnotationWhenArraySizeGreater,
    SchemaCompilerAnnotationToParent, SchemaCompilerAnnotationBasenameToParent,
    SchemaCompilerLogicalOr, SchemaCompilerLogicalAnd, SchemaCompilerLogicalXor,
    SchemaCompilerLogicalCondition, SchemaCompilerLogicalNot,
    SchemaCompilerLogicalWhenType, SchemaCompilerLogicalWhenDefines,
    SchemaCompilerLogicalWhenArraySizeGreater,
    SchemaCompilerLogicalWhenArraySizeEqual, SchemaCompilerLoopPropertiesMatch,
    SchemaCompilerLoopProperties, SchemaCompilerLoopPropertiesRegex,
    SchemaCompilerLoopPropertiesNoAnnotation,
    SchemaCompilerLoopPropertiesExcept, SchemaCompilerLoopPropertiesType,
    SchemaCompilerLoopPropertiesTypeStrict, SchemaCompilerLoopKeys,
    SchemaCompilerLoopItems, SchemaCompilerLoopItemsUnmarked,
    SchemaCompilerLoopItemsUnevaluated, SchemaCompilerLoopContains,
    SchemaCompilerControlLabel, SchemaCompilerControlMark,
    SchemaCompilerControlJump, SchemaCompilerControlDynamicAnchorJump>>;

#if !defined(DOXYGEN)
// For fast internal instruction dispatching. It must stay
// in sync with the variant ordering above
enum class SchemaCompilerTemplateIndex : std::uint8_t {
  SchemaCompilerAssertionFail = 0,
  SchemaCompilerAssertionDefines,
  SchemaCompilerAssertionDefinesAll,
  SchemaCompilerAssertionPropertyDependencies,
  SchemaCompilerAssertionType,
  SchemaCompilerAssertionTypeAny,
  SchemaCompilerAssertionTypeStrict,
  SchemaCompilerAssertionTypeStrictAny,
  SchemaCompilerAssertionTypeStringBounded,
  SchemaCompilerAssertionTypeArrayBounded,
  SchemaCompilerAssertionTypeObjectBounded,
  SchemaCompilerAssertionRegex,
  SchemaCompilerAssertionStringSizeLess,
  SchemaCompilerAssertionStringSizeGreater,
  SchemaCompilerAssertionArraySizeLess,
  SchemaCompilerAssertionArraySizeGreater,
  SchemaCompilerAssertionObjectSizeLess,
  SchemaCompilerAssertionObjectSizeGreater,
  SchemaCompilerAssertionEqual,
  SchemaCompilerAssertionEqualsAny,
  SchemaCompilerAssertionGreaterEqual,
  SchemaCompilerAssertionLessEqual,
  SchemaCompilerAssertionGreater,
  SchemaCompilerAssertionLess,
  SchemaCompilerAssertionUnique,
  SchemaCompilerAssertionDivisible,
  SchemaCompilerAssertionStringType,
  SchemaCompilerAssertionPropertyType,
  SchemaCompilerAssertionPropertyTypeStrict,
  SchemaCompilerAnnotationEmit,
  SchemaCompilerAnnotationWhenArraySizeEqual,
  SchemaCompilerAnnotationWhenArraySizeGreater,
  SchemaCompilerAnnotationToParent,
  SchemaCompilerAnnotationBasenameToParent,
  SchemaCompilerLogicalOr,
  SchemaCompilerLogicalAnd,
  SchemaCompilerLogicalXor,
  SchemaCompilerLogicalCondition,
  SchemaCompilerLogicalNot,
  SchemaCompilerLogicalWhenType,
  SchemaCompilerLogicalWhenDefines,
  SchemaCompilerLogicalWhenArraySizeGreater,
  SchemaCompilerLogicalWhenArraySizeEqual,
  SchemaCompilerLoopPropertiesMatch,
  SchemaCompilerLoopProperties,
  SchemaCompilerLoopPropertiesRegex,
  SchemaCompilerLoopPropertiesNoAnnotation,
  SchemaCompilerLoopPropertiesExcept,
  SchemaCompilerLoopPropertiesType,
  SchemaCompilerLoopPropertiesTypeStrict,
  SchemaCompilerLoopKeys,
  SchemaCompilerLoopItems,
  SchemaCompilerLoopItemsUnmarked,
  SchemaCompilerLoopItemsUnevaluated,
  SchemaCompilerLoopContains,
  SchemaCompilerControlLabel,
  SchemaCompilerControlMark,
  SchemaCompilerControlJump,
  SchemaCompilerControlDynamicAnchorJump
};
#endif

#define DEFINE_STEP_WITH_VALUE(category, name, type)                           \
  struct SchemaCompiler##category##name {                                      \
    const Pointer relative_schema_location;                                    \
    const Pointer relative_instance_location;                                  \
    const std::string keyword_location;                                        \
    const std::string schema_resource;                                         \
    const bool dynamic;                                                        \
    const bool report;                                                         \
    const type value;                                                          \
  };

#define DEFINE_STEP_APPLICATOR(category, name, type)                           \
  struct SchemaCompiler##category##name {                                      \
    const Pointer relative_schema_location;                                    \
    const Pointer relative_instance_location;                                  \
    const std::string keyword_location;                                        \
    const std::string schema_resource;                                         \
    const bool dynamic;                                                        \
    const bool report;                                                         \
    const type value;                                                          \
    const SchemaCompilerTemplate children;                                     \
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
DEFINE_STEP_WITH_VALUE(Assertion, Fail, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a given property
DEFINE_STEP_WITH_VALUE(Assertion, Defines, SchemaCompilerValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties
DEFINE_STEP_WITH_VALUE(Assertion, DefinesAll, SchemaCompilerValueStrings)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties if it defines other set of properties
DEFINE_STEP_WITH_VALUE(Assertion, PropertyDependencies,
                       SchemaCompilerValueStringMap)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type
DEFINE_STEP_WITH_VALUE(Assertion, Type, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types
DEFINE_STEP_WITH_VALUE(Assertion, TypeAny, SchemaCompilerValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrict, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrictAny, SchemaCompilerValueTypes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type string and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeStringBounded, SchemaCompilerValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type array and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeArrayBounded, SchemaCompilerValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// type object and adheres to the given bounds
DEFINE_STEP_WITH_VALUE(Assertion, TypeObjectBounded, SchemaCompilerValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a string against an
/// ECMA regular expression
DEFINE_STEP_WITH_VALUE(Assertion, Regex, SchemaCompilerValueRegex)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// less than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// greater than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// less than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// greater than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// less than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// greater than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks the instance equals
/// a given JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Equal, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that a JSON document
/// is equal to at least one of the given elements
DEFINE_STEP_WITH_VALUE(Assertion, EqualsAny, SchemaCompilerValueArray)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, GreaterEqual, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, LessEqual, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Greater, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Less, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a given JSON array
/// does not contain duplicate items
DEFINE_STEP_WITH_VALUE(Assertion, Unique, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks a number is
/// divisible by another number
DEFINE_STEP_WITH_VALUE(Assertion, Divisible, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that a string is of
/// a certain type
DEFINE_STEP_WITH_VALUE(Assertion, StringType, SchemaCompilerValueStringType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present
DEFINE_STEP_WITH_VALUE(Assertion, PropertyType, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present (strict mode)
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrict, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation
DEFINE_STEP_WITH_VALUE(Annotation, Emit, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation when the size of
/// the array instance is equal to the given size
DEFINE_STEP_WITH_VALUE(Annotation, WhenArraySizeEqual,
                       SchemaCompilerValueIndexedJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation when the size of
/// the array instance is greater than the given size
DEFINE_STEP_WITH_VALUE(Annotation, WhenArraySizeGreater,
                       SchemaCompilerValueIndexedJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits an annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, ToParent, SchemaCompilerValueJSON)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that emits the current basename as an
/// annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, BasenameToParent, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a disjunction
DEFINE_STEP_APPLICATOR(Logical, Or, SchemaCompilerValueBoolean)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction
DEFINE_STEP_APPLICATOR(Logical, And, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents an exclusive
/// disjunction
DEFINE_STEP_APPLICATOR(Logical, Xor, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents an imperative conditional compiler logical step
DEFINE_STEP_APPLICATOR(Logical, Condition, SchemaCompilerValueIndexPair)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a negation
DEFINE_STEP_APPLICATOR(Logical, Not, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is of a given type
DEFINE_STEP_APPLICATOR(Logical, WhenType, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is an object and defines a given property
DEFINE_STEP_APPLICATOR(Logical, WhenDefines, SchemaCompilerValueString)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the array instance size is greater than the given number
DEFINE_STEP_APPLICATOR(Logical, WhenArraySizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the array instance size is equal to the given number
DEFINE_STEP_APPLICATOR(Logical, WhenArraySizeEqual,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that matches steps to object properties
DEFINE_STEP_APPLICATOR(Loop, PropertiesMatch, SchemaCompilerValueNamedIndexes)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties
DEFINE_STEP_APPLICATOR(Loop, Properties, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// match a given ECMA regular expression
DEFINE_STEP_APPLICATOR(Loop, PropertiesRegex, SchemaCompilerValueRegex)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// were not collected as annotations
DEFINE_STEP_APPLICATOR(Loop, PropertiesNoAnnotation, SchemaCompilerValueStrings)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object properties that
/// do not match the given property filters
DEFINE_STEP_APPLICATOR(Loop, PropertiesExcept,
                       SchemaCompilerValuePropertyFilter)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type
DEFINE_STEP_WITH_VALUE(Loop, PropertiesType, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks every object property is of a
/// given type (strict mode)
DEFINE_STEP_WITH_VALUE(Loop, PropertiesTypeStrict, SchemaCompilerValueType)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over object property keys
DEFINE_STEP_APPLICATOR(Loop, Keys, SchemaCompilerValueNone)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items starting from
/// a given index
DEFINE_STEP_APPLICATOR(Loop, Items, SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over array items when the array
/// is considered unmarked
DEFINE_STEP_APPLICATOR(Loop, ItemsUnmarked, SchemaCompilerValueStrings)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that loops over unevaluated array items
DEFINE_STEP_APPLICATOR(Loop, ItemsUnevaluated,
                       SchemaCompilerValueItemsAnnotationKeywords)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that checks array items match a given
/// criteria
DEFINE_STEP_APPLICATOR(Loop, Contains, SchemaCompilerValueRange)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of a mark to jump to while
/// executing children instructions
DEFINE_STEP_APPLICATOR(Control, Label, SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of a mark to jump to, but
/// without executing children instructions
DEFINE_STEP_APPLICATOR(Control, Mark, SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of jumping into a
/// pre-registered label
DEFINE_STEP_WITH_VALUE(Control, Jump, SchemaCompilerValueUnsignedInteger)

/// @ingroup evaluator_instructions
/// @brief Represents a compiler step that consists of jump to a dynamic anchor
DEFINE_STEP_WITH_VALUE(Control, DynamicAnchorJump, SchemaCompilerValueString)

#undef DEFINE_STEP_WITH_VALUE
#undef DEFINE_STEP_APPLICATOR

} // namespace sourcemeta::jsontoolkit

#endif
