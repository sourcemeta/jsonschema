#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_

#if defined(__EMSCRIPTEN__) || defined(__Unikraft__)
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#else
#include "jsonschema_export.h"
#endif

#include <sourcemeta/jsontoolkit/jsonschema_reference.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <functional> // std::function
#include <map>        // std::map
#include <optional>   // std::optional, std::nullopt
#include <regex>      // std::regex
#include <set>        // std::set
#include <string>     // std::string
#include <utility>    // std::move, std::pair
#include <variant>    // std::variant
#include <vector>     // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
/// Represents a type of compiler step target
enum class SchemaCompilerTargetType {
  /// An static instance literal
  Instance,

  /// The last path (i.e. property or index) of the instance location
  InstanceBasename,

  /// The penultimate path (i.e. property or index) of the instance location
  InstanceParent,

  /// The annotations produced at the same base evaluation path for the parent
  /// of the current instance location
  ParentAdjacentAnnotations
};

/// @ingroup jsonschema
/// Represents a generic compiler step target
using SchemaCompilerTarget = std::pair<SchemaCompilerTargetType, Pointer>;

/// @ingroup jsonschema
/// Represents a compiler step empty value
struct SchemaCompilerValueNone {};

/// @ingroup jsonschema
/// Represents a compiler step JSON value
using SchemaCompilerValueJSON = JSON;

/// @ingroup jsonschema
/// Represents a set of JSON values
using SchemaCompilerValueArray = std::set<JSON>;

/// @ingroup jsonschema
/// Represents a compiler step string value
using SchemaCompilerValueString = JSON::String;

/// @ingroup jsonschema
/// Represents a compiler step string values
using SchemaCompilerValueStrings = std::set<JSON::String>;

/// @ingroup jsonschema
/// Represents a compiler step JSON type value
using SchemaCompilerValueType = JSON::Type;

/// @ingroup jsonschema
/// Represents a compiler step JSON types value
using SchemaCompilerValueTypes = std::set<JSON::Type>;

/// @ingroup jsonschema
/// Represents a compiler step ECMA regular expression value. We store both the
/// original string and the regular expression as standard regular expressions
/// do not keep a copy of their original value (which we need for serialization
/// purposes)
using SchemaCompilerValueRegex = std::pair<std::regex, std::string>;

/// @ingroup jsonschema
/// Represents a compiler step JSON unsigned integer value
using SchemaCompilerValueUnsignedInteger = std::size_t;

/// @ingroup jsonschema
/// Represents a compiler step boolean value
using SchemaCompilerValueBoolean = bool;

/// @ingroup jsonschema
/// Represents a compiler step a string logical type
enum class SchemaCompilerValueStringType { URI };

/// @ingroup jsonschema
/// Represents a value in a compiler step
template <typename T>
using SchemaCompilerStepValue = std::variant<T, SchemaCompilerTarget>;

/// @ingroup jsonschema
/// Represents a compiler assertion step that always fails
struct SchemaCompilerAssertionFail;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if an object defines a
/// given property
struct SchemaCompilerAssertionDefines;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if an object defines a
/// set of properties
struct SchemaCompilerAssertionDefinesAll;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if a document is of the
/// given type
struct SchemaCompilerAssertionType;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if a document is of any of
/// the given types
struct SchemaCompilerAssertionTypeAny;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if a document is of the
/// given type (strict version)
struct SchemaCompilerAssertionTypeStrict;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks if a document is of any of
/// the given types (strict version)
struct SchemaCompilerAssertionTypeStrictAny;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a string against an ECMA
/// regular expression
struct SchemaCompilerAssertionRegex;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a given array, object, or
/// string has at least a certain number of items, properties, or characters,
/// respectively
struct SchemaCompilerAssertionSizeGreater;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a given array, object, or
/// string has less than a certain number of items, properties, or characters,
/// respectively
struct SchemaCompilerAssertionSizeLess;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks the instance equals a given
/// JSON document
struct SchemaCompilerAssertionEqual;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks that a JSON document is
/// equal to at least one of the given elements
struct SchemaCompilerAssertionEqualsAny;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a JSON document is greater
/// than or equal to another JSON document
struct SchemaCompilerAssertionGreaterEqual;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a JSON document is less
/// than or equal to another JSON document
struct SchemaCompilerAssertionLessEqual;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a JSON document is greater
/// than another JSON document
struct SchemaCompilerAssertionGreater;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a JSON document is less
/// than another JSON document
struct SchemaCompilerAssertionLess;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a given JSON array does not
/// contain duplicate items
struct SchemaCompilerAssertionUnique;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks a number is divisible by
/// another number
struct SchemaCompilerAssertionDivisible;

/// @ingroup jsonschema
/// Represents a compiler assertion step that checks that a string is of a
/// certain type
struct SchemaCompilerAssertionStringType;

/// @ingroup jsonschema
/// Represents a compiler step that emits a public annotation
struct SchemaCompilerAnnotationPublic;

/// @ingroup jsonschema
/// Represents a compiler step that emits a private annotation
struct SchemaCompilerAnnotationPrivate;

/// @ingroup jsonschema
/// Represents a compiler logical step that represents a disjunction
struct SchemaCompilerLogicalOr;

/// @ingroup jsonschema
/// Represents a compiler logical step that represents a conjunction
struct SchemaCompilerLogicalAnd;

/// @ingroup jsonschema
/// Represents a compiler logical step that represents an exclusive disjunction
struct SchemaCompilerLogicalXor;

/// @ingroup jsonschema
/// Represents a compiler logical step that represents a negation
struct SchemaCompilerLogicalNot;

/// @ingroup jsonschema
/// Represents a hidden compiler assertion step that checks a certain
/// annotation was not produced
struct SchemaCompilerInternalNoAnnotation;

/// @ingroup jsonschema
/// Represents a hidden conjunction compiler step
struct SchemaCompilerInternalContainer;

/// @ingroup jsonschema
/// Represents a hidden compiler assertion step that checks if an object defines
/// a set of properties
struct SchemaCompilerInternalDefinesAll;

/// @ingroup jsonschema
/// Represents a compiler step that loops over object properties
struct SchemaCompilerLoopProperties;

/// @ingroup jsonschema
/// Represents a compiler step that loops over object property keys
struct SchemaCompilerLoopKeys;

/// @ingroup jsonschema
/// Represents a compiler step that loops over array items starting from a given
/// index
struct SchemaCompilerLoopItems;

/// @ingroup jsonschema
/// Represents a compiler step that checks array items match a given criteria
struct SchemaCompilerLoopContains;

/// @ingroup jsonschema
/// Represents a compiler step that consists of a mark to jump to
struct SchemaCompilerControlLabel;

/// @ingroup jsonschema
/// Represents a compiler step that consists of jumping into a pre-registered
/// label
struct SchemaCompilerControlJump;

/// @ingroup jsonschema
/// Represents a schema compilation step that can be evaluated
using SchemaCompilerTemplate = std::vector<std::variant<
    SchemaCompilerAssertionFail, SchemaCompilerAssertionDefines,
    SchemaCompilerAssertionDefinesAll, SchemaCompilerAssertionType,
    SchemaCompilerAssertionTypeAny, SchemaCompilerAssertionTypeStrict,
    SchemaCompilerAssertionTypeStrictAny, SchemaCompilerAssertionRegex,
    SchemaCompilerAssertionSizeGreater, SchemaCompilerAssertionSizeLess,
    SchemaCompilerAssertionEqual, SchemaCompilerAssertionEqualsAny,
    SchemaCompilerAssertionGreaterEqual, SchemaCompilerAssertionLessEqual,
    SchemaCompilerAssertionGreater, SchemaCompilerAssertionLess,
    SchemaCompilerAssertionUnique, SchemaCompilerAssertionDivisible,
    SchemaCompilerAssertionStringType, SchemaCompilerAnnotationPublic,
    SchemaCompilerAnnotationPrivate, SchemaCompilerLogicalOr,
    SchemaCompilerLogicalAnd, SchemaCompilerLogicalXor,
    SchemaCompilerLogicalNot, SchemaCompilerInternalNoAnnotation,
    SchemaCompilerInternalContainer, SchemaCompilerInternalDefinesAll,
    SchemaCompilerLoopProperties, SchemaCompilerLoopKeys,
    SchemaCompilerLoopItems, SchemaCompilerLoopContains,
    SchemaCompilerControlLabel, SchemaCompilerControlJump>>;

#if !defined(DOXYGEN)
#define DEFINE_STEP_WITH_VALUE(category, name, type)                           \
  struct SchemaCompiler##category##name {                                      \
    const SchemaCompilerTarget target;                                         \
    const Pointer relative_schema_location;                                    \
    const Pointer relative_instance_location;                                  \
    const std::string keyword_location;                                        \
    const SchemaCompilerStepValue<type> value;                                 \
    const SchemaCompilerTemplate condition;                                    \
  };

#define DEFINE_STEP_APPLICATOR(category, name, type)                           \
  struct SchemaCompiler##category##name {                                      \
    const SchemaCompilerTarget target;                                         \
    const Pointer relative_schema_location;                                    \
    const Pointer relative_instance_location;                                  \
    const std::string keyword_location;                                        \
    const SchemaCompilerStepValue<type> value;                                 \
    const SchemaCompilerTemplate children;                                     \
    const SchemaCompilerTemplate condition;                                    \
  };

#define DEFINE_CONTROL(name)                                                   \
  struct SchemaCompilerControl##name {                                         \
    const Pointer relative_schema_location;                                    \
    const Pointer relative_instance_location;                                  \
    const std::string keyword_location;                                        \
    const std::size_t id;                                                      \
    const SchemaCompilerTemplate children;                                     \
  };

DEFINE_STEP_WITH_VALUE(Assertion, Fail, SchemaCompilerValueNone)
DEFINE_STEP_WITH_VALUE(Assertion, Defines, SchemaCompilerValueString)
DEFINE_STEP_WITH_VALUE(Assertion, DefinesAll, SchemaCompilerValueStrings)
DEFINE_STEP_WITH_VALUE(Assertion, Type, SchemaCompilerValueType)
DEFINE_STEP_WITH_VALUE(Assertion, TypeAny, SchemaCompilerValueTypes)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrict, SchemaCompilerValueType)
DEFINE_STEP_WITH_VALUE(Assertion, TypeStrictAny, SchemaCompilerValueTypes)
DEFINE_STEP_WITH_VALUE(Assertion, Regex, SchemaCompilerValueRegex)
DEFINE_STEP_WITH_VALUE(Assertion, SizeGreater,
                       SchemaCompilerValueUnsignedInteger)
DEFINE_STEP_WITH_VALUE(Assertion, SizeLess, SchemaCompilerValueUnsignedInteger)
DEFINE_STEP_WITH_VALUE(Assertion, Equal, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, EqualsAny, SchemaCompilerValueArray)
DEFINE_STEP_WITH_VALUE(Assertion, GreaterEqual, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, LessEqual, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, Greater, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, Less, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, Unique, SchemaCompilerValueNone)
DEFINE_STEP_WITH_VALUE(Assertion, Divisible, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Assertion, StringType, SchemaCompilerValueStringType)
DEFINE_STEP_WITH_VALUE(Annotation, Public, SchemaCompilerValueJSON)
DEFINE_STEP_WITH_VALUE(Annotation, Private, SchemaCompilerValueJSON)
DEFINE_STEP_APPLICATOR(Logical, Or, SchemaCompilerValueNone)
DEFINE_STEP_APPLICATOR(Logical, And, SchemaCompilerValueNone)
DEFINE_STEP_APPLICATOR(Logical, Xor, SchemaCompilerValueNone)
DEFINE_STEP_APPLICATOR(Logical, Not, SchemaCompilerValueNone)
DEFINE_STEP_WITH_VALUE(Internal, NoAnnotation, SchemaCompilerValueJSON)
DEFINE_STEP_APPLICATOR(Internal, Container, SchemaCompilerValueNone)
DEFINE_STEP_WITH_VALUE(Internal, DefinesAll, SchemaCompilerValueStrings)
DEFINE_STEP_APPLICATOR(Loop, Properties, SchemaCompilerValueBoolean)
DEFINE_STEP_APPLICATOR(Loop, Keys, SchemaCompilerValueNone)
DEFINE_STEP_APPLICATOR(Loop, Items, SchemaCompilerValueUnsignedInteger)
DEFINE_STEP_APPLICATOR(Loop, Contains, SchemaCompilerValueNone)
DEFINE_CONTROL(Label)
DEFINE_CONTROL(Jump)

#undef DEFINE_STEP_WITH_VALUE
#undef DEFINE_STEP_APPLICATOR
#undef DEFINE_CONTROL
#endif

#if !defined(DOXYGEN)
struct SchemaCompilerContext;
#endif

/// @ingroup jsonschema
/// A compiler is represented as a function that maps a keyword compiler context
/// into a compiler template. You can provide your own to implement your own
/// keywords
using SchemaCompiler =
    std::function<SchemaCompilerTemplate(const SchemaCompilerContext &)>;

/// @ingroup jsonschema
/// The compiler context is the information you have at your disposal to
/// implement a keyword
struct SchemaCompilerContext {
  /// The schema keyword
  const std::string keyword;
  /// The current subschema
  const JSON &schema;
  /// The schema vocabularies in use
  const std::map<std::string, bool> &vocabularies;
  /// The value of the keyword
  const JSON &value;
  /// The root schema resource
  const JSON &root;
  /// The schema base URI
  const URI base;
  /// The schema location relative to the base URI
  const Pointer relative_pointer;
  /// The schema base keyword path
  const Pointer base_schema_location;
  /// The base instance location that the keyword must be evaluated to
  const Pointer base_instance_location;
  /// The set of labels registered so far
  const std::set<std::size_t> labels;
  /// The reference frame of the entire schema
  const ReferenceFrame &frame;
  /// The references of the entire schema
  const ReferenceMap &references;
  /// The schema walker in use
  const SchemaWalker walker;
  /// The schema resolver in use
  const SchemaResolver resolver;
  /// The schema compiler in use
  const SchemaCompiler compiler;
  /// The default dialect of the schema
  const std::optional<std::string> &default_dialect;
};

/// @ingroup jsonschema
/// Represents the mode of evalution
enum class SchemaCompilerEvaluationMode {
  /// Attempt to get to a boolean result as fast as possible, ignoring
  /// everything that is not strictly required (like collecting most
  /// annotations)
  Fast,
  /// Perform a full schema evaluation
  Exhaustive
};

/// @ingroup jsonschema
/// A callback of this type is invoked after evaluating any keyword. The
/// arguments go as follows:
///
/// - Whether the evaluation was successful or not
/// - The step that was just evaluated
/// - The evaluation path
/// - The instance location
/// - The instance document
/// - The annotation result, if any (otherwise null)
///
/// You can use this callback mechanism to implement arbitrary output formats.
using SchemaCompilerEvaluationCallback = std::function<void(
    bool, const SchemaCompilerTemplate::value_type &, const Pointer &,
    const Pointer &, const JSON &, const JSON &)>;

/// @ingroup jsonschema
///
/// This function translates a step execution into a human-readable string.
/// Useful as the building block for producing user-friendly evaluation results.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
describe(const SchemaCompilerTemplate::value_type &step) -> std::string;

// TODO: Support standard output formats. Maybe through pre-made evaluation
// callbacks?

/// @ingroup jsonschema
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

/// @ingroup jsonschema
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

/// @ingroup jsonschema
/// A default compiler that aims to implement every keyword for official JSON
/// Schema dialects.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT default_schema_compiler(
    const SchemaCompilerContext &) -> SchemaCompilerTemplate;

/// @ingroup jsonschema
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

/// @ingroup jsonschema
///
/// This function compiles a single subschema into a compiler template as
/// determined by the given pointer. If a URI is given, the compiler will
/// attempt to jump to that corresponding frame entry. Otherwise, it will
/// navigate within the current keyword. This function is not meant to be used
/// directly, but instead as a building block for supporting applicators on
/// compiler functions.
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
compile(const SchemaCompilerContext &context, const Pointer &schema_suffix,
        const Pointer &instance_suffix = empty_pointer,
        const std::optional<std::string> &uri = std::nullopt)
    -> SchemaCompilerTemplate;

/// @ingroup jsonschema
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

} // namespace sourcemeta::jsontoolkit

#endif
