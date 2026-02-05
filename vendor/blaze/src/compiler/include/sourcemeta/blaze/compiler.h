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

#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint8_t
#include <functional>    // std::function
#include <map>           // std::map
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

/// @defgroup compiler Compiler
/// @brief Compile a JSON Schema into a set of low-level instructions for fast
/// evaluation

namespace sourcemeta::blaze {

/// @ingroup compiler
/// The schema compiler context is the current subschema information you have at
/// your disposal to implement a keyword
struct SchemaContext {
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  /// The schema location relative to the base URI
  const sourcemeta::core::WeakPointer &relative_pointer;
  /// The current subschema
  const sourcemeta::core::JSON &schema;
  /// The schema vocabularies in use
  const sourcemeta::core::Vocabularies &vocabularies;
  /// The schema base URI
  const sourcemeta::core::URI &base;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
  /// Whether the current schema targets a property name
  bool is_property_name;
};

/// @ingroup compiler
/// The dynamic compiler context is the read-write information you have at your
/// disposal to implement a keyword
struct DynamicContext {
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
  /// The schema keyword
  const sourcemeta::core::JSON::String &keyword;
  /// The schema base keyword path
  const sourcemeta::core::WeakPointer &base_schema_location;
  /// The base instance location that the keyword must be evaluated to
  const sourcemeta::core::WeakPointer &base_instance_location;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
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
  /// Always unroll `properties` in a logical AND operation
  bool properties_always_unroll{false};
  /// Attempt to re-order `properties` subschemas to evaluate cheaper ones first
  bool properties_reorder{true};
  /// Inline jump targets with fewer instructions than this threshold
  std::size_t target_inline_threshold{50};
};

/// @ingroup compiler
/// The static compiler context is the information you have at your
/// disposal to implement a keyword that will never change throughout
/// the compilation process
struct Context {
  // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
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
  /// The set of tweaks for the compiler
  const Tweaks tweaks;
  /// All possible reference targets (key includes is_property_name context)
  const std::map<
      std::tuple<sourcemeta::core::SchemaReferenceType, std::string_view, bool>,
      std::pair<std::size_t, const sourcemeta::core::WeakPointer *>>
      targets;
  // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
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
///     schema, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver,
///     sourcemeta::core::default_schema_compiler)};
///
/// // Evaluate or encode
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
compile(const sourcemeta::core::JSON &schema,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const Compiler &compiler, const Mode mode = Mode::FastValidation,
        const std::string_view default_dialect = "",
        const std::string_view default_id = "",
        const std::string_view entrypoint = "",
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
auto SOURCEMETA_BLAZE_COMPILER_EXPORT compile(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver, const Compiler &compiler,
    const sourcemeta::core::SchemaFrame &frame,
    const std::string_view entrypoint, const Mode mode = Mode::FastValidation,
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
        const sourcemeta::core::WeakPointer &schema_suffix,
        const sourcemeta::core::WeakPointer &instance_suffix =
            sourcemeta::core::empty_weak_pointer,
        std::optional<std::string_view> uri = std::nullopt) -> Instructions;

/// @ingroup compiler
/// Serialise a template as JSON
auto SOURCEMETA_BLAZE_COMPILER_EXPORT to_json(const Template &schema_template)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
