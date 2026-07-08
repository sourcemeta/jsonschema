#ifndef SOURCEMETA_BLAZE_OUTPUT_JSONLD_H_
#define SOURCEMETA_BLAZE_OUTPUT_JSONLD_H_

#ifndef SOURCEMETA_BLAZE_OUTPUT_EXPORT
#include <sourcemeta/blaze/output_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output_simple.h>

#include <array>   // std::array
#include <cstdint> // std::uint8_t
#include <string>  // std::string
#include <variant> // std::variant
#include <vector>  // std::vector

namespace sourcemeta::blaze {

/// @ingroup output
/// The x-jsonld-* keywords that jsonld() resolves. Use these as the annotation
/// whitelist when compiling a schema so that its annotations are collected.
inline constexpr std::array<sourcemeta::core::JSON::StringView, 10>
    JSONLD_KEYWORDS{{"x-jsonld-id", "x-jsonld-type", "x-jsonld-reverse",
                     "x-jsonld-datatype", "x-jsonld-language",
                     "x-jsonld-direction", "x-jsonld-json", "x-jsonld-graph",
                     "x-jsonld-container", "x-jsonld-self"}};

/// @ingroup output
/// The descriptor facet that a JSON-LD resolution error is about
enum class JSONLDFacet : std::uint8_t {
  Type,
  Predicate,
  Datatype,
  Language,
  Direction,
  Graph,
  JSON,
  Container,
  Self
};

/// @ingroup output
/// The instance conforms but one of its JSON-LD annotations cannot be resolved
/// into a consistent mapping. Reported as a single failure at the instance
/// location where resolution stopped, distinct from a schema validation error.
struct JSONLDResolutionError {
  sourcemeta::core::Pointer instance_location;
  JSONLDFacet facet;
  std::string message;
};

/// @ingroup output
/// The instance does not conform to the schema, so no JSON-LD is produced.
/// These are the schema validation errors.
using JSONLDInvalid = std::vector<SimpleOutput::Entry>;

/// @ingroup output
/// The tri-state outcome of a JSON-LD evaluation: the expanded document (as
/// expanded JSON-LD), schema validation errors, or a JSON-LD resolution error.
using JSONLDOutcome =
    std::variant<sourcemeta::core::JSON, JSONLDInvalid, JSONLDResolutionError>;

/// @ingroup output
///
/// Evaluate an instance against a JSON Schema whose atoms carry JSON-LD
/// annotations and, on success, return the instance promoted to expanded
/// JSON-LD. The schema must have been compiled with annotation collection
/// enabled for its JSON-LD keywords, as shown below. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
/// #include <sourcemeta/blaze/output.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// #include <unordered_set>
/// #include <variant>
///
/// const sourcemeta::core::JSON schema =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object",
///   "x-jsonld-type": "https://schema.org/Person",
///   "properties": {
///     "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" }
///   }
/// })JSON");
///
/// sourcemeta::blaze::Tweaks tweaks;
/// tweaks.annotations = std::unordered_set<sourcemeta::core::JSON::StringView>(
///     sourcemeta::blaze::JSONLD_KEYWORDS.begin(),
///     sourcemeta::blaze::JSONLD_KEYWORDS.end());
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::blaze::schema_walker,
///     sourcemeta::blaze::schema_resolver,
///     sourcemeta::blaze::default_schema_compiler,
///     sourcemeta::blaze::Mode::FastValidation, "", "", "", tweaks)};
///
/// const sourcemeta::core::JSON instance{
///     sourcemeta::core::parse_json(R"JSON({ "name": "Ada" })JSON")};
///
/// sourcemeta::blaze::Evaluator evaluator;
/// const auto outcome{
///     sourcemeta::blaze::jsonld(evaluator, schema_template, instance)};
/// ```
auto SOURCEMETA_BLAZE_OUTPUT_EXPORT
jsonld(Evaluator &evaluator, const Template &schema,
       const sourcemeta::core::JSON &instance) -> JSONLDOutcome;

} // namespace sourcemeta::blaze

#endif
