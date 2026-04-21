#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_H_
#define SOURCEMETA_BLAZE_ALTERSCHEMA_H_

/// @defgroup alterschema AlterSchema
/// @brief A growing collection of JSON Schema transformation rules.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/alterschema.h>
/// ```

#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
#include <sourcemeta/blaze/alterschema_export.h>
#endif

#include <sourcemeta/blaze/alterschema_error.h>
#include <sourcemeta/blaze/alterschema_transformer.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <type_traits> // std::false_type

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup alterschema
/// The category of a built-in transformation rule
enum class AlterSchemaMode : std::uint8_t {
  /// Rules that simplify the given schema for both human readability and
  /// performance
  Linter,

  /// Rules that surface implicit constraints and simplifies keywords that
  /// are syntax sugar to other keywords, potentially decreasing human
  /// readability in favor of explicitness
  Canonicalizer,
};

/// @ingroup alterschema
/// Add a set of built-in schema transformation rules given a category. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/blaze/alterschema.h>
///
/// sourcemeta::blaze::SchemaTransformer bundle;
///
/// sourcemeta::blaze::add(bundle,
///   sourcemeta::blaze::AlterSchemaMode::Linter);
///
/// auto schema = sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// bundle.apply(schema, sourcemeta::core::schema_walker,
///              sourcemeta::core::schema_resolver);
/// ```
SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void;

/// @ingroup alterschema
///
/// A linter rule driven by a JSON Schema. Every subschema in the document
/// under inspection is validated as a JSON instance against the provided
/// rule schema. When a subschema does not conform, the rule fires and
/// reports the validation errors. The rule name is extracted from the
/// `title` keyword of the rule schema, and the rule description from the
/// `description` keyword. The title must consist only of lowercase ASCII
/// letters, digits, underscores, or slashes.
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaRule final
    : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  SchemaRule(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const std::string_view default_dialect = "",
             const std::optional<Tweaks> &tweaks = std::nullopt);
  [[nodiscard]] auto condition(const sourcemeta::core::JSON &,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::Vocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override;

private:
  Template template_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
