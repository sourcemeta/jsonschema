#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/blaze/compiler_error.h>
#include <sourcemeta/blaze/compiler_output.h>

#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <cstdint>    // std::uint8_t
#include <functional> // std::function
#include <map>        // std::map
#include <optional>   // std::optional, std::nullopt
#include <set>        // std::set
#include <string>     // std::string
#include <vector>     // std::vector

/// @defgroup compiler Compiler
/// @brief Compile a JSON Schema into a set of low-level instructions for fast
/// evaluation

namespace sourcemeta::blaze {

/// @ingroup compiler
/// The schema compiler context is the current subschema information you have at
/// your disposal to implement a keyword
struct SchemaContext {
  /// The schema location relative to the base URI
  const sourcemeta::jsontoolkit::Pointer &relative_pointer;
  /// The current subschema
  const sourcemeta::jsontoolkit::JSON &schema;
  /// The schema vocabularies in use
  const std::map<std::string, bool> &vocabularies;
  /// The schema base URI
  const sourcemeta::jsontoolkit::URI &base;
  /// The set of labels registered so far
  std::set<std::size_t> labels;
  /// The set of references destinations traversed so far
  std::set<std::string> references;
};

/// @ingroup compiler
/// The dynamic compiler context is the read-write information you have at your
/// disposal to implement a keyword
struct DynamicContext {
  /// The schema keyword
  const std::string &keyword;
  /// The schema base keyword path
  const sourcemeta::jsontoolkit::Pointer &base_schema_location;
  /// The base instance location that the keyword must be evaluated to
  const sourcemeta::jsontoolkit::Pointer &base_instance_location;
};

#if !defined(DOXYGEN)
struct Context;
#endif

/// @ingroup compiler
/// A compiler is represented as a function that maps a keyword compiler
/// contexts into a compiler template. You can provide your own to implement
/// your own keywords
using Compiler = std::function<Template(const Context &, const SchemaContext &,
                                        const DynamicContext &)>;

/// @ingroup evaluator
/// Represents the mode of compilation
enum class Mode : std::uint8_t {
  /// Attempt to get to a boolean result as fast as possible
  FastValidation,
  /// Perform exhaustive evaluation, including annotations
  Exhaustive
};

/// @ingroup compiler
/// The static compiler context is the information you have at your
/// disposal to implement a keyword that will never change throughout
/// the compilation process
struct Context {
  /// The root schema resource
  const sourcemeta::jsontoolkit::JSON &root;
  /// The reference frame of the entire schema
  const sourcemeta::jsontoolkit::ReferenceFrame &frame;
  /// The references of the entire schema
  const sourcemeta::jsontoolkit::ReferenceMap &references;
  /// The set of all schema resources in the schema without duplicates
  const std::vector<std::string> resources;
  /// The schema walker in use
  const sourcemeta::jsontoolkit::SchemaWalker &walker;
  /// The schema resolver in use
  const sourcemeta::jsontoolkit::SchemaResolver &resolver;
  /// The schema compiler in use
  const Compiler &compiler;
  /// The mode of the schema compiler
  const Mode mode;
  /// Whether the schema makes use of dynamic scoping
  const bool uses_dynamic_scopes;
  /// The list of subschemas that require keeping track of unevaluated
  /// properties
  const std::set<sourcemeta::jsontoolkit::Pointer>
      unevaluated_properties_schemas;
  /// The list of subschemas that require keeping track of unevaluated items
  const std::set<sourcemeta::jsontoolkit::Pointer> unevaluated_items_schemas;
  /// The list of subschemas that are precompiled at the beginning of the
  /// instruction set
  const std::set<std::string> precompiled_static_schemas;
};

/// @ingroup compiler
/// A default compiler that aims to implement every keyword for official JSON
/// Schema dialects.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT default_schema_compiler(
    const Context &, const SchemaContext &, const DynamicContext &) -> Template;

/// @ingroup compiler
///
/// This function compiles an input JSON Schema into a template that can be
/// later evaluated. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
///
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// // Evaluate or encode
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
compile(const sourcemeta::jsontoolkit::JSON &schema,
        const sourcemeta::jsontoolkit::SchemaWalker &walker,
        const sourcemeta::jsontoolkit::SchemaResolver &resolver,
        const Compiler &compiler, const Mode mode = Mode::FastValidation,
        const std::optional<std::string> &default_dialect = std::nullopt)
    -> Template;

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
        const sourcemeta::jsontoolkit::Pointer &schema_suffix,
        const sourcemeta::jsontoolkit::Pointer &instance_suffix =
            sourcemeta::jsontoolkit::empty_pointer,
        const std::optional<std::string> &uri = std::nullopt) -> Template;

/// @ingroup compiler
///
/// This function converts a compiler template into JSON. Convenient for storing
/// it or sending it over the wire. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
///
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
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON result{
///     sourcemeta::blaze::to_json(schema_template)};
///
/// sourcemeta::jsontoolkit::prettify(result, std::cout);
/// std::cout << "\n";
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT to_json(const Template &steps)
    -> sourcemeta::jsontoolkit::JSON;

/// @ingroup compiler
///
/// An opinionated key comparison for printing JSON Schema compiler templates
/// with sourcemeta::jsontoolkit::prettify or
/// sourcemeta::jsontoolkit::stringify. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
///
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
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON result{
///     sourcemeta::blaze::to_json(schema_template)};
///
/// sourcemeta::jsontoolkit::prettify(result, std::cout,
/// template_format_compare); std::cout << "\n";
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT template_format_compare(
    const sourcemeta::jsontoolkit::JSON::String &left,
    const sourcemeta::jsontoolkit::JSON::String &right) -> bool;

} // namespace sourcemeta::blaze

#endif
