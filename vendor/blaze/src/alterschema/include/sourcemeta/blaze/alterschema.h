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
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>

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

  /// Rules that upgrade a JSON Schema document up to JSON Schema Draft 4
  UpgradeDraft4,

  /// Rules that upgrade a JSON Schema document up to JSON Schema Draft 6
  UpgradeDraft6,

  /// Rules that upgrade a JSON Schema document up to JSON Schema Draft 7
  UpgradeDraft7,

  /// Rules that upgrade a JSON Schema document up to JSON Schema 2019-09
  Upgrade201909,

  /// Rules that upgrade a JSON Schema document up to JSON Schema 2020-12
  Upgrade202012,
};

/// @ingroup alterschema
/// Add a set of built-in schema transformation rules given a category. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/blaze/foundation.h>
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
/// bundle.apply(schema, sourcemeta::blaze::schema_walker,
///              sourcemeta::blaze::schema_resolver);
/// ```
SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void;

/// @ingroup alterschema
///
/// A linter rule driven by a JSON Schema. By default, every subschema in
/// the document under inspection is validated as a JSON instance against
/// the provided rule schema, though a rule may be scoped to only run
/// against the document root. When a subschema does not conform, the rule
/// fires and reports the validation errors. The rule name is extracted
/// from the `title` keyword of the rule schema, and the rule description
/// from the `description` keyword. The title must consist only of
/// lowercase ASCII letters, digits, underscores, or slashes.
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaRule final
    : public SchemaTransformRule {
public:
  /// The locations of the schema that the rule applies to
  enum class Scope : std::uint8_t {
    /// Every subschema of the document under inspection
    All,

    /// Only the document root
    TopLevel
  };

  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  SchemaRule(const sourcemeta::core::JSON &schema,
             const sourcemeta::blaze::SchemaWalker &walker,
             const sourcemeta::blaze::SchemaResolver &resolver,
             const Compiler &compiler,
             const std::string_view default_dialect = "",
             const std::optional<Tweaks> &tweaks = std::nullopt,
             const Scope scope = Scope::All);
  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &, const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override;

private:
  Template template_;
  Scope scope_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

/// @ingroup alterschema
///
/// Wrap a schema to only access one of its subschemas. This is useful if you
/// want to perform validation on only a specific part of the schema without
/// having to reinvent the wheel. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/alterschema.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "type": "string" }
/// })JSON");
///
/// sourcemeta::blaze::SchemaFrame frame{
///     sourcemeta::blaze::SchemaFrame::Mode::References};
/// frame.analyse(document, sourcemeta::blaze::schema_walker,
///               sourcemeta::blaze::schema_resolver);
///
/// const auto location{frame.traverse(
///     sourcemeta::core::WeakPointer{"items"},
///     sourcemeta::blaze::SchemaFrame::LocationType::Subschema)};
///
/// sourcemeta::core::WeakPointer base;
/// const sourcemeta::core::JSON result =
///   sourcemeta::blaze::wrap(document, frame, location.value().get(),
///     sourcemeta::blaze::schema_resolver, base);
///
/// sourcemeta::core::prettify(result, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
auto wrap(const sourcemeta::core::JSON &schema, const SchemaFrame &frame,
          const SchemaFrame::Location &location, const SchemaResolver &resolver,
          sourcemeta::core::WeakPointer &base) -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
