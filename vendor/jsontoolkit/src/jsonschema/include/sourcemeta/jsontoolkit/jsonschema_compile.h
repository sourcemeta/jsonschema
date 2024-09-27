#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_COMPILE_H_

#include "jsonschema_export.h"

#include <sourcemeta/jsontoolkit/evaluator.h>
#include <sourcemeta/jsontoolkit/jsonschema_reference.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <functional> // std::function
#include <map>        // std::map
#include <optional>   // std::optional, std::nullopt
#include <set>        // std::set
#include <string>     // std::string
#include <vector>     // std::vector

/// @ingroup jsonschema
/// @defgroup jsonschema_compiler Compiler
/// @brief Compile a JSON Schema into a set of low-level instructions for fast
/// evaluation

namespace sourcemeta::jsontoolkit {

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
                  const WeakPointer &instance_location, const JSON &annotation)
      -> void;

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
