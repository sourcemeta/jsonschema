#ifndef SOURCEMETA_BLAZE_COMPILER_COMPILE_H_
#define SOURCEMETA_BLAZE_COMPILER_COMPILE_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include "compiler_export.h"
#endif

#include <sourcemeta/blaze/compiler_error.h>
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
  /// Whether the schema makes use of unevaluated properties
  const bool uses_unevaluated_properties;
  /// Whether the schema makes use of unevaluated items
  const bool uses_unevaluated_items;
};

/// @ingroup compiler
///
/// A simple evaluation callback that reports a stack trace in the case of
/// validation error that you can report as you with. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
///
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// #include <cassert>
/// #include <functional>
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
/// const sourcemeta::jsontoolkit::JSON instance{5};
///
/// sourcemeta::blaze::ErrorTraceOutput output;
/// const auto result{sourcemeta::blaze::evaluate(
///   schema_template, instance, std::ref(output))};
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
class SOURCEMETA_BLAZE_COMPILER_EXPORT ErrorTraceOutput {
public:
  ErrorTraceOutput(const sourcemeta::jsontoolkit::JSON &instance,
                   const sourcemeta::jsontoolkit::WeakPointer &base =
                       sourcemeta::jsontoolkit::empty_weak_pointer);

  // Prevent accidental copies
  ErrorTraceOutput(const ErrorTraceOutput &) = delete;
  auto operator=(const ErrorTraceOutput &) -> ErrorTraceOutput & = delete;

  struct Entry {
    const std::string message;
    const sourcemeta::jsontoolkit::WeakPointer instance_location;
    const sourcemeta::jsontoolkit::WeakPointer evaluate_path;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Template::value_type &step,
                  const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
                  const sourcemeta::jsontoolkit::WeakPointer &instance_location,
                  const sourcemeta::jsontoolkit::JSON &annotation) -> void;

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
  const sourcemeta::jsontoolkit::JSON &instance_;
  const sourcemeta::jsontoolkit::WeakPointer base_;
  container_type output;
  std::set<sourcemeta::jsontoolkit::WeakPointer> mask;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup compiler
///
/// This function translates a step execution into a human-readable string.
/// Useful as the building block for producing user-friendly evaluation results.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
describe(const bool valid, const Template::value_type &step,
         const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
         const sourcemeta::jsontoolkit::WeakPointer &instance_location,
         const sourcemeta::jsontoolkit::JSON &instance,
         const sourcemeta::jsontoolkit::JSON &annotation) -> std::string;

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
