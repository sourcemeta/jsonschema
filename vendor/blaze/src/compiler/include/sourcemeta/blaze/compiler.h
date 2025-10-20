#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/blaze/compiler_error.h>
#include <sourcemeta/blaze/compiler_unevaluated.h>

#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <cstdint>       // std::uint8_t
#include <functional>    // std::function
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

/// @defgroup compiler Compiler
/// @brief Compile a JSON Schema into a set of low-level instructions for fast
/// evaluation

namespace sourcemeta::blaze {

/// @ingroup compiler
/// The schema compiler context is the current subschema information you have at
/// your disposal to implement a keyword
struct SchemaContext {
  /// The schema location relative to the base URI
  const sourcemeta::core::Pointer &relative_pointer;
  /// The current subschema
  const sourcemeta::core::JSON &schema;
  /// The schema vocabularies in use
  const sourcemeta::core::Vocabularies &vocabularies;
  /// The schema base URI
  const sourcemeta::core::URI &base;
  /// The set of labels registered so far
  std::unordered_set<std::size_t> labels;
  /// Whether the current schema targets a property name
  bool is_property_name;
};

/// @ingroup compiler
/// The dynamic compiler context is the read-write information you have at your
/// disposal to implement a keyword
struct DynamicContext {
  /// The schema keyword
  const std::string keyword;
  /// The schema base keyword path
  const sourcemeta::core::Pointer &base_schema_location;
  /// The base instance location that the keyword must be evaluated to
  const sourcemeta::core::Pointer &base_instance_location;
  /// Whether the instance location property acts as the target
  const bool property_as_target;
};

#if !defined(DOXYGEN)
struct Context;
#endif

/// @ingroup compiler
/// A compiler is represented as a function that maps a keyword compiler
/// contexts into a compiler template. You can provide your own to implement
/// your own keywords
using Compiler =
    std::function<Instructions(const Context &, const SchemaContext &,
                               const DynamicContext &, const Instructions &)>;

/// @ingroup evaluator
/// Represents the mode of compilation
enum class Mode : std::uint8_t {
  /// Attempt to get to a boolean result as fast as possible
  FastValidation,
  /// Perform exhaustive evaluation, including annotations
  Exhaustive
};

/// @ingroup compiler
/// Advanced knobs that you can tweak for higher control and optimisations
struct Tweaks {
  /// Consider static references that are not circular when precompiling static
  /// references
  bool precompile_static_references_non_circular{false};
  /// The maximum amount of static references to precompile
  std::size_t precompile_static_references_maximum_schemas{10};
  /// The minimum amount of references to a destination before considering it
  /// for precompilation
  std::size_t precompile_static_references_minimum_reference_count{10};
  /// Always unroll `properties` in a logical AND operation
  bool properties_always_unroll{false};
  /// Attempt to re-order `properties` subschemas to evaluate cheaper ones first
  bool properties_reorder{true};
};

/// @ingroup compiler
/// The static compiler context is the information you have at your
/// disposal to implement a keyword that will never change throughout
/// the compilation process
struct Context {
  /// The root schema resource
  const sourcemeta::core::JSON &root;
  /// The reference frame of the entire schema
  const sourcemeta::core::SchemaFrame &frame;
  /// The set of all schema resources in the schema without duplicates
  const std::vector<std::string> resources;
  /// The schema walker in use
  const sourcemeta::core::SchemaWalker &walker;
  /// The schema resolver in use
  const sourcemeta::core::SchemaResolver &resolver;
  /// The schema compiler in use
  const Compiler &compiler;
  /// The mode of the schema compiler
  const Mode mode;
  /// Whether the schema makes use of dynamic scoping
  const bool uses_dynamic_scopes;
  /// The list of unevaluated entries and their dependencies
  const SchemaUnevaluatedEntries unevaluated;
  /// The set of global labels identifier during precompilation
  std::unordered_set<std::size_t> precompiled_labels;
  /// The set of global labels identifier during precompilation
  const Tweaks tweaks;
};

/// @ingroup compiler
/// A default compiler that aims to implement every keyword for official JSON
/// Schema dialects.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT default_schema_compiler(
    const Context &, const SchemaContext &, const DynamicContext &,
    const Instructions &) -> Instructions;

/// @ingroup compiler
///
/// This function compiles an input JSON Schema into a template that can be
/// later evaluated. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// const sourcemeta::core::JSON schema =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::core::schema_official_walker,
///     sourcemeta::core::schema_official_resolver,
///     sourcemeta::core::default_schema_compiler)};
///
/// // Evaluate or encode
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
compile(const sourcemeta::core::JSON &schema,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const Compiler &compiler, const Mode mode = Mode::FastValidation,
        const std::optional<std::string> &default_dialect = std::nullopt,
        const std::optional<std::string> &default_id = std::nullopt,
        const std::optional<Tweaks> &tweaks = std::nullopt) -> Template;

/// @ingroup compiler
///
/// This function compiles an input JSON Schema into a template that can be
/// later evaluated, but given an existing schema frame. The schema frame must
/// contain reference information for the given schema and the input schema must
/// be bundled. If those pre-conditions are not met, you will hit undefined
/// behavior.
///
/// Don't use this function unless you know what you are doing.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
compile(const sourcemeta::core::JSON &schema,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const Compiler &compiler, const sourcemeta::core::SchemaFrame &frame,
        const Mode mode = Mode::FastValidation,
        const std::optional<std::string> &default_dialect = std::nullopt,
        const std::optional<std::string> &default_id = std::nullopt,
        const std::optional<Tweaks> &tweaks = std::nullopt) -> Template;

/// @ingroup compiler
///
/// This function compiles a single subschema into a compiler template as
/// determined by the given pointer. If a URI is given, the compiler will
/// attempt to jump to that corresponding frame entry. Otherwise, it will
/// navigate within the current keyword. This function is not meant to be used
/// directly, but instead as a building block for supporting applicators on
/// compiler functions.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
compile(const Context &context, const SchemaContext &schema_context,
        const DynamicContext &dynamic_context,
        const sourcemeta::core::Pointer &schema_suffix,
        const sourcemeta::core::Pointer &instance_suffix =
            sourcemeta::core::empty_pointer,
        const std::optional<std::string> &uri = std::nullopt) -> Instructions;

/// @ingroup compiler
/// Serialise a template as JSON
auto SOURCEMETA_BLAZE_COMPILER_EXPORT to_json(const Template &schema_template)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
