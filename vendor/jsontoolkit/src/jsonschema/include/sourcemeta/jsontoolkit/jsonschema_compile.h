#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_

#include "jsonschema_export.h"

#include <sourcemeta/jsontoolkit/jsonschema_reference.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <functional>    // std::function
#include <map>           // std::map
#include <optional>      // std::optional, std::nullopt
#include <regex>         // std::regex
#include <set>           // std::set
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::pair
#include <variant>       // std::variant
#include <vector>        // std::vector

/// @ingroup jsonschema
/// @defgroup jsonschema_compiler Compiler
/// @brief Compile a JSON Schema into a set of low-level instructions for fast
/// evaluation

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema_compiler
/// @brief Represents a compiler step empty value
struct SchemaCompilerValueNone {};

/// @ingroup jsonschema_compiler
/// Represents a compiler step JSON value
using SchemaCompilerValueJSON = JSON;

// Note that for these steps, we prefer vectors over sets as the former performs
// better for small collections, where we can even guarantee uniqueness when
// generating the instructions

/// @ingroup jsonschema_compiler
/// Represents a set of JSON values
using SchemaCompilerValueArray = std::vector<JSON>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step string values
using SchemaCompilerValueStrings = std::vector<JSON::String>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step JSON types value
using SchemaCompilerValueTypes = std::vector<JSON::Type>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step string value
using SchemaCompilerValueString = JSON::String;

/// @ingroup jsonschema_compiler
/// Represents a compiler step JSON type value
using SchemaCompilerValueType = JSON::Type;

/// @ingroup jsonschema_compiler
/// Represents a compiler step ECMA regular expression value. We store both the
/// original string and the regular expression as standard regular expressions
/// do not keep a copy of their original value (which we need for serialization
/// purposes)
using SchemaCompilerValueRegex = std::pair<std::regex, std::string>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step JSON unsigned integer value
using SchemaCompilerValueUnsignedInteger = std::size_t;

/// @ingroup jsonschema_compiler
/// Represents a compiler step range value. The boolean option
/// modifies whether the range is considered exhaustively or
/// if the evaluator is allowed to break early
using SchemaCompilerValueRange =
    std::tuple<std::size_t, std::optional<std::size_t>, bool>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step boolean value
using SchemaCompilerValueBoolean = bool;

/// @ingroup jsonschema_compiler
/// Represents a compiler step string to index map
using SchemaCompilerValueNamedIndexes =
    std::unordered_map<SchemaCompilerValueString,
                       SchemaCompilerValueUnsignedInteger>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step string logical type
enum class SchemaCompilerValueStringType { URI };

/// @ingroup jsonschema_compiler
/// Represents an array loop compiler step annotation keywords
struct SchemaCompilerValueItemsAnnotationKeywords {
  const SchemaCompilerValueString index;
  const SchemaCompilerValueStrings filter;
  const SchemaCompilerValueStrings mask;
};

/// @ingroup jsonschema_compiler
/// Represents an compiler step that maps strings to strings
using SchemaCompilerValueStringMap =
    std::unordered_map<SchemaCompilerValueString, SchemaCompilerValueStrings>;

/// @ingroup jsonschema_compiler
/// Represents a compiler step JSON value accompanied with an index
using SchemaCompilerValueIndexedJSON =
    std::pair<SchemaCompilerValueUnsignedInteger, JSON>;

// Note that while we generally avoid sets, in this case, we want
// hash-based lookups on string collections that might get large.
/// @ingroup jsonschema_compiler
/// Represents a compiler step value that consist of object property filters
using SchemaCompilerValuePropertyFilter =
    std::pair<std::set<SchemaCompilerValueString>,
              std::vector<SchemaCompilerValueRegex>>;

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
struct SchemaCompilerLogicalTryMark;
struct SchemaCompilerLogicalNot;
struct SchemaCompilerLogicalWhenType;
struct SchemaCompilerLogicalWhenDefines;
struct SchemaCompilerLogicalWhenAdjacentUnmarked;
struct SchemaCompilerLogicalWhenAdjacentMarked;
struct SchemaCompilerLogicalWhenArraySizeGreater;
struct SchemaCompilerLogicalWhenArraySizeEqual;
struct SchemaCompilerLoopPropertiesMatch;
struct SchemaCompilerLoopProperties;
struct SchemaCompilerLoopPropertiesRegex;
struct SchemaCompilerLoopPropertiesNoAnnotation;
struct SchemaCompilerLoopPropertiesExcept;
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

/// @ingroup jsonschema_compiler
/// Represents a schema compilation step that can be evaluated
using SchemaCompilerTemplate = std::vector<std::variant<
    SchemaCompilerAssertionFail, SchemaCompilerAssertionDefines,
    SchemaCompilerAssertionDefinesAll,
    SchemaCompilerAssertionPropertyDependencies, SchemaCompilerAssertionType,
    SchemaCompilerAssertionTypeAny, SchemaCompilerAssertionTypeStrict,
    SchemaCompilerAssertionTypeStrictAny, SchemaCompilerAssertionRegex,
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
    SchemaCompilerLogicalTryMark, SchemaCompilerLogicalNot,
    SchemaCompilerLogicalWhenType, SchemaCompilerLogicalWhenDefines,
    SchemaCompilerLogicalWhenAdjacentUnmarked,
    SchemaCompilerLogicalWhenAdjacentMarked,
    SchemaCompilerLogicalWhenArraySizeGreater,
    SchemaCompilerLogicalWhenArraySizeEqual, SchemaCompilerLoopPropertiesMatch,
    SchemaCompilerLoopProperties, SchemaCompilerLoopPropertiesRegex,
    SchemaCompilerLoopPropertiesNoAnnotation,
    SchemaCompilerLoopPropertiesExcept, SchemaCompilerLoopKeys,
    SchemaCompilerLoopItems, SchemaCompilerLoopItemsUnmarked,
    SchemaCompilerLoopItemsUnevaluated, SchemaCompilerLoopContains,
    SchemaCompilerControlLabel, SchemaCompilerControlMark,
    SchemaCompilerControlJump, SchemaCompilerControlDynamicAnchorJump>>;

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

/// @defgroup jsonschema_compiler_instructions Instruction Set
/// @ingroup jsonschema_compiler
/// @brief The set of instructions supported by the compiler.
/// @details
///
/// Every instruction operates at a specific instance location and with the
/// given value, whose type depends on the instruction.

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that always fails
DEFINE_STEP_WITH_VALUE(Assertion, Fail, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a given property
DEFINE_STEP_WITH_VALUE(Assertion, Defines, SchemaCompilerValueString)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties
DEFINE_STEP_WITH_VALUE(Assertion, DefinesAll, SchemaCompilerValueStrings)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if an object defines
/// a set of properties if it defines other set of properties
DEFINE_STEP_WITH_VALUE(Assertion, PropertyDependencies,
                       SchemaCompilerValueStringMap)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type
DEFINE_STEP_WITH_VALUE(Assertion, Type, SchemaCompilerValueType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types
DEFINE_STEP_WITH_VALUE(Assertion, TypeAny, SchemaCompilerValueTypes)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// the given type (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrict, SchemaCompilerValueType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks if a document is of
/// any of the given types (strict version)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrictAny, SchemaCompilerValueTypes)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a string against an
/// ECMA regular expression
DEFINE_STEP_WITH_VALUE(Assertion, Regex, SchemaCompilerValueRegex)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// less than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given string has
/// greater than a certain number of characters
DEFINE_STEP_WITH_VALUE(Assertion, StringSizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// less than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given array has
/// greater than a certain number of items
DEFINE_STEP_WITH_VALUE(Assertion, ArraySizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// less than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeLess,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given object has
/// greater than a certain number of properties
DEFINE_STEP_WITH_VALUE(Assertion, ObjectSizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks the instance equals
/// a given JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Equal, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks that a JSON document
/// is equal to at least one of the given elements
DEFINE_STEP_WITH_VALUE(Assertion, EqualsAny, SchemaCompilerValueArray)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, GreaterEqual, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than or equal to another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, LessEqual, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// greater than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Greater, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a JSON document is
/// less than another JSON document
DEFINE_STEP_WITH_VALUE(Assertion, Less, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a given JSON array
/// does not contain duplicate items
DEFINE_STEP_WITH_VALUE(Assertion, Unique, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks a number is
/// divisible by another number
DEFINE_STEP_WITH_VALUE(Assertion, Divisible, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks that a string is of
/// a certain type
DEFINE_STEP_WITH_VALUE(Assertion, StringType, SchemaCompilerValueStringType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present
DEFINE_STEP_WITH_VALUE(Assertion, PropertyType, SchemaCompilerValueType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler assertion step that checks that an instance
/// property is of a given type if present (strict mode)
DEFINE_STEP_WITH_VALUE(Assertion, PropertyTypeStrict, SchemaCompilerValueType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that emits an annotation
DEFINE_STEP_WITH_VALUE(Annotation, Emit, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that emits an annotation when the size of
/// the array instance is equal to the given size
DEFINE_STEP_WITH_VALUE(Annotation, WhenArraySizeEqual,
                       SchemaCompilerValueIndexedJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that emits an annotation when the size of
/// the array instance is greater than the given size
DEFINE_STEP_WITH_VALUE(Annotation, WhenArraySizeGreater,
                       SchemaCompilerValueIndexedJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that emits an annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, ToParent, SchemaCompilerValueJSON)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that emits the current basename as an
/// annotation to the parent
DEFINE_STEP_WITH_VALUE(Annotation, BasenameToParent, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a disjunction
DEFINE_STEP_APPLICATOR(Logical, Or, SchemaCompilerValueBoolean)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction
DEFINE_STEP_APPLICATOR(Logical, And, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents an exclusive
/// disjunction
DEFINE_STEP_APPLICATOR(Logical, Xor, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction that
/// always reports success and marks its outcome for other steps
DEFINE_STEP_APPLICATOR(Logical, TryMark, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a negation
DEFINE_STEP_APPLICATOR(Logical, Not, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is of a given type
DEFINE_STEP_APPLICATOR(Logical, WhenType, SchemaCompilerValueType)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance is an object and defines a given property
DEFINE_STEP_APPLICATOR(Logical, WhenDefines, SchemaCompilerValueString)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance and desired evaluation path was not marked
DEFINE_STEP_APPLICATOR(Logical, WhenAdjacentUnmarked, SchemaCompilerValueString)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the instance and desired evaluation path was marked
DEFINE_STEP_APPLICATOR(Logical, WhenAdjacentMarked, SchemaCompilerValueString)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the array instance size is greater than the given number
DEFINE_STEP_APPLICATOR(Logical, WhenArraySizeGreater,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler logical step that represents a conjunction when
/// the array instance size is equal to the given number
DEFINE_STEP_APPLICATOR(Logical, WhenArraySizeEqual,
                       SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that matches steps to object properties
DEFINE_STEP_APPLICATOR(Loop, PropertiesMatch, SchemaCompilerValueNamedIndexes)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over object properties
DEFINE_STEP_APPLICATOR(Loop, Properties, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over object properties that
/// match a given ECMA regular expression
DEFINE_STEP_APPLICATOR(Loop, PropertiesRegex, SchemaCompilerValueRegex)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over object properties that
/// were not collected as annotations
DEFINE_STEP_APPLICATOR(Loop, PropertiesNoAnnotation, SchemaCompilerValueStrings)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over object properties that
/// do not match the given property filters
DEFINE_STEP_APPLICATOR(Loop, PropertiesExcept,
                       SchemaCompilerValuePropertyFilter)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over object property keys
DEFINE_STEP_APPLICATOR(Loop, Keys, SchemaCompilerValueNone)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over array items starting from
/// a given index
DEFINE_STEP_APPLICATOR(Loop, Items, SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over array items when the array
/// is considered unmarked
DEFINE_STEP_APPLICATOR(Loop, ItemsUnmarked, SchemaCompilerValueStrings)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that loops over unevaluated array items
DEFINE_STEP_APPLICATOR(Loop, ItemsUnevaluated,
                       SchemaCompilerValueItemsAnnotationKeywords)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that checks array items match a given
/// criteria
DEFINE_STEP_APPLICATOR(Loop, Contains, SchemaCompilerValueRange)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that consists of a mark to jump to while
/// executing children instructions
DEFINE_STEP_APPLICATOR(Control, Label, SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that consists of a mark to jump to, but
/// without executing children instructions
DEFINE_STEP_APPLICATOR(Control, Mark, SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that consists of jumping into a
/// pre-registered label
DEFINE_STEP_WITH_VALUE(Control, Jump, SchemaCompilerValueUnsignedInteger)

/// @ingroup jsonschema_compiler_instructions
/// @brief Represents a compiler step that consists of jump to a dynamic anchor
DEFINE_STEP_WITH_VALUE(Control, DynamicAnchorJump, SchemaCompilerValueString)

#undef DEFINE_STEP_WITH_VALUE
#undef DEFINE_STEP_APPLICATOR

/// @ingroup jsonschema_compiler
/// The schema compiler context is the current subschema information you have at
/// your disposal to implement a keyword
struct SchemaCompilerSchemaContext {
  /// The schema location relative to the base URI
  const Pointer &relative_pointer;
  /// The current subschema
  const JSON &schema;
  /// The schema vocabularies in use
  const std::map<std::string, bool> &vocabularies;
  /// The schema base URI
  const URI &base;
  /// The set of labels registered so far
  std::set<std::size_t> labels;
  /// The set of references destinations traversed so far
  std::set<std::string> references;
};

/// @ingroup jsonschema_compiler
/// The dynamic compiler context is the read-write information you have at your
/// disposal to implement a keyword
struct SchemaCompilerDynamicContext {
  /// The schema keyword
  const std::string &keyword;
  /// The schema base keyword path
  const Pointer &base_schema_location;
  /// The base instance location that the keyword must be evaluated to
  const Pointer &base_instance_location;
};

#if !defined(DOXYGEN)
struct SchemaCompilerContext;
#endif

/// @ingroup jsonschema_compiler
/// A compiler is represented as a function that maps a keyword compiler
/// contexts into a compiler template. You can provide your own to implement
/// your own keywords
using SchemaCompiler = std::function<SchemaCompilerTemplate(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &)>;

/// @ingroup jsonschema_compiler
/// The static compiler context is the information you have at your
/// disposal to implement a keyword that will never change throughout
/// the compilation process
struct SchemaCompilerContext {
  /// The root schema resource
  const JSON &root;
  /// The reference frame of the entire schema
  const ReferenceFrame &frame;
  /// The references of the entire schema
  const ReferenceMap &references;
  /// The schema walker in use
  const SchemaWalker &walker;
  /// The schema resolver in use
  const SchemaResolver &resolver;
  /// The schema compiler in use
  const SchemaCompiler &compiler;
  /// Whether the schema makes use of dynamic scoping
  const bool uses_dynamic_scopes;
};

/// @ingroup jsonschema_compiler
/// Represents the mode of evalution
enum class SchemaCompilerEvaluationMode {
  /// Attempt to get to a boolean result as fast as possible
  Fast,
  /// Perform a full schema evaluation
  Exhaustive
};

/// @ingroup jsonschema_compiler
/// Represents the state of a step evaluation
enum class SchemaCompilerEvaluationType { Pre, Post };

/// @ingroup jsonschema_compiler
/// A callback of this type is invoked after evaluating any keyword. The
/// arguments go as follows:
///
/// - The stage at which the step in question is
/// - Whether the evaluation was successful or not (always true before
/// evaluation)
/// - The step that was just evaluated
/// - The evaluation path
/// - The instance location
/// - The annotation result, if any (otherwise null)
///
/// You can use this callback mechanism to implement arbitrary output formats.
using SchemaCompilerEvaluationCallback =
    std::function<void(const SchemaCompilerEvaluationType, bool,
                       const SchemaCompilerTemplate::value_type &,
                       const WeakPointer &, const WeakPointer &, const JSON &)>;

// TODO: Support standard output formats too

/// @ingroup jsonschema_compiler
///
/// A simple evaluation callback that reports a stack trace in the case of
/// validation error that you can report as you with. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
/// #include <functional>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON instance{5};
///
/// sourcemeta::jsontoolkit::SchemaCompilerErrorTraceOutput output;
/// const auto result{sourcemeta::jsontoolkit::evaluate(
///   schema_template, instance,
///   sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
///   std::ref(output))};
///
/// if (!result) {
///   for (const auto &trace : output) {
///     std::cerr << trace.message << "\n";
///     sourcemeta::jsontoolkit::stringify(trace.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::jsontoolkit::stringify(trace.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaCompilerErrorTraceOutput {
public:
  SchemaCompilerErrorTraceOutput(const JSON &instance,
                                 const WeakPointer &base = empty_weak_pointer);

  // Prevent accidental copies
  SchemaCompilerErrorTraceOutput(const SchemaCompilerErrorTraceOutput &) =
      delete;
  auto operator=(const SchemaCompilerErrorTraceOutput &)
      -> SchemaCompilerErrorTraceOutput & = delete;

  struct Entry {
    const std::string message;
    const WeakPointer instance_location;
    const WeakPointer evaluate_path;
  };

  auto operator()(const SchemaCompilerEvaluationType type, const bool result,
                  const SchemaCompilerTemplate::value_type &step,
                  const WeakPointer &evaluate_path,
                  const WeakPointer &instance_location,
                  const JSON &annotation) -> void;

  using container_type = typename std::vector<Entry>;
  using const_iterator = typename container_type::const_iterator;
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const JSON &instance_;
  const WeakPointer base_;
  container_type output;
  std::set<WeakPointer> mask;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup jsonschema_compiler
///
/// This function translates a step execution into a human-readable string.
/// Useful as the building block for producing user-friendly evaluation results.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
describe(const bool valid, const SchemaCompilerTemplate::value_type &step,
         const WeakPointer &evaluate_path, const WeakPointer &instance_location,
         const JSON &instance, const JSON &annotation) -> std::string;

/// @ingroup jsonschema_compiler
///
/// This function evaluates a schema compiler template in validation mode,
/// returning a boolean without error information. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON instance{"foo bar"};
/// const auto result{sourcemeta::jsontoolkit::evaluate(
///   schema_template, instance)};
/// assert(result);
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
evaluate(const SchemaCompilerTemplate &steps, const JSON &instance) -> bool;

/// @ingroup jsonschema_compiler
///
/// This function evaluates a schema compiler template, executing the given
/// callback at every step of the way. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
/// #include <iostream>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// static auto callback(
///     bool result,
///     const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step,
///     const sourcemeta::jsontoolkit::Pointer &evaluate_path,
///     const sourcemeta::jsontoolkit::Pointer &instance_location,
///     const sourcemeta::jsontoolkit::JSON &document,
///     const sourcemeta::jsontoolkit::JSON &annotation) -> void {
///   std::cout << "TYPE: " << (result ? "Success" : "Failure") << "\n";
///   std::cout << "STEP:\n";
///   sourcemeta::jsontoolkit::prettify(sourcemeta::jsontoolkit::to_json({step}),
///                                     std::cout);
///   std::cout << "\nEVALUATE PATH:";
///   sourcemeta::jsontoolkit::stringify(evaluate_path, std::cout);
///   std::cout << "\nINSTANCE LOCATION:";
///   sourcemeta::jsontoolkit::stringify(instance_location, std::cout);
///   std::cout << "\nANNOTATION:\n";
///   sourcemeta::jsontoolkit::prettify(annotation, std::cout);
///   std::cout << "\n";
/// }
///
/// const sourcemeta::jsontoolkit::JSON instance{"foo bar"};
/// const auto result{sourcemeta::jsontoolkit::evaluate(
///   schema_template, instance,
///   sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
///   callback)};
///
/// assert(result);
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
evaluate(const SchemaCompilerTemplate &steps, const JSON &instance,
         const SchemaCompilerEvaluationMode mode,
         const SchemaCompilerEvaluationCallback &callback) -> bool;

/// @ingroup jsonschema_compiler
/// A default compiler that aims to implement every keyword for official JSON
/// Schema dialects.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT default_schema_compiler(
    const SchemaCompilerContext &, const SchemaCompilerSchemaContext &,
    const SchemaCompilerDynamicContext &) -> SchemaCompilerTemplate;

/// @ingroup jsonschema_compiler
///
/// This function compiles an input JSON Schema into a template that can be
/// later evaluated. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// // Evaluate or encode
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
compile(const JSON &schema, const SchemaWalker &walker,
        const SchemaResolver &resolver, const SchemaCompiler &compiler,
        const std::optional<std::string> &default_dialect = std::nullopt)
    -> SchemaCompilerTemplate;

/// @ingroup jsonschema_compiler
///
/// This function compiles a single subschema into a compiler template as
/// determined by the given pointer. If a URI is given, the compiler will
/// attempt to jump to that corresponding frame entry. Otherwise, it will
/// navigate within the current keyword. This function is not meant to be used
/// directly, but instead as a building block for supporting applicators on
/// compiler functions.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
compile(const SchemaCompilerContext &context,
        const SchemaCompilerSchemaContext &schema_context,
        const SchemaCompilerDynamicContext &dynamic_context,
        const Pointer &schema_suffix,
        const Pointer &instance_suffix = empty_pointer,
        const std::optional<std::string> &uri = std::nullopt)
    -> SchemaCompilerTemplate;

/// @ingroup jsonschema_compiler
///
/// This function converts a compiler template into JSON. Convenient for storing
/// it or sending it over the wire. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON result{
///     sourcemeta::jsontoolkit::to_json(schema_template)};
///
/// sourcemeta::jsontoolkit::prettify(result, std::cout);
/// std::cout << "\n";
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
to_json(const SchemaCompilerTemplate &steps) -> JSON;

/// @ingroup jsonschema_compiler
///
/// An opinionated key comparison for printing JSON Schema compiler templates
/// with sourcemeta::jsontoolkit::prettify or
/// sourcemeta::jsontoolkit::stringify. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::jsontoolkit::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON result{
///     sourcemeta::jsontoolkit::to_json(schema_template)};
///
/// sourcemeta::jsontoolkit::prettify(result, std::cout,
/// compiler_template_format_compare); std::cout << "\n";
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT compiler_template_format_compare(
    const JSON::String &left, const JSON::String &right) -> bool;

} // namespace sourcemeta::jsontoolkit

#endif
