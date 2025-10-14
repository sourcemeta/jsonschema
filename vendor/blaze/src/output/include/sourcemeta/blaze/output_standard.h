#ifndef SOURCEMETA_BLAZE_OUTPUT_STANDARD_H_
#define SOURCEMETA_BLAZE_OUTPUT_STANDARD_H_

#ifndef SOURCEMETA_BLAZE_OUTPUT_EXPORT
#include <sourcemeta/blaze/output_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>

namespace sourcemeta::blaze {

/// @ingroup output
/// Represents standard output formats
/// See
/// https://json-schema.org/draft/2020-12/json-schema-core#name-output-structure
/// See
/// https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.10
enum class StandardOutput {
  Flag,
  Basic
  // TODO: Implement the "detailed" and "verbose" output formats
};

// TODO: Integrate with
// https://github.com/json-schema-org/JSON-Schema-Test-Suite/tree/main/output-tests

/// @ingroup output
/// Perform JSON Schema evaluation using Standard Output formats. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
/// #include <sourcemeta/blaze/output.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// #include <cassert>
/// #include <iostream>
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
/// const sourcemeta::core::JSON instance{"foo bar"};
///
/// sourcemeta::blaze::Evaluator evaluator;
///
/// const auto result{sourcemeta::blaze::standard(
///   evaluator, schema_template, instance,
///   sourcemeta::blaze::StandardOutput::Basic)};
///
/// assert(result.is_object());
/// assert(result.defines("valid"));
/// assert(result.at("valid").is_boolean());
/// assert(result.at("valid").to_boolean());
///
/// sourcemeta::core::prettify(result,
///   std::cout, sourcemeta::blaze::standard_output_compare);
/// std::cout << "\n";
/// ```
///
/// Note that this output format is not a class like the others
/// in order to have additional control over how and whether to
/// pass a callback to the evaluator instance.
auto SOURCEMETA_BLAZE_OUTPUT_EXPORT
standard(Evaluator &evaluator, const Template &schema,
         const sourcemeta::core::JSON &instance, const StandardOutput format)
    -> sourcemeta::core::JSON;

/// @ingroup output
///
/// An overload of the standard output function that includes line and column
/// position information as an extension.
auto SOURCEMETA_BLAZE_OUTPUT_EXPORT
standard(Evaluator &evaluator, const Template &schema,
         const sourcemeta::core::JSON &instance, const StandardOutput format,
         const sourcemeta::core::PointerPositionTracker &instanceTracker)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
