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
/// const auto result{bundle.apply(schema,
///   sourcemeta::core::schema_walker,
///   sourcemeta::core::schema_resolver,
///   [](const auto &pointer,
///      const auto &name,
///      const auto &message,
///      const auto &outcome,
///      const auto dismissed) {
///     // Do something with the information
///   })};
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
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const std::string_view default_dialect = "",
             const std::optional<Tweaks> &tweaks = std::nullopt,
             const Scope scope = Scope::All);
  [[nodiscard]] auto condition(const sourcemeta::core::JSON &,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::SchemaVocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
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
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/blaze/alterschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "type": "string" }
/// })JSON");
///
/// const sourcemeta::core::SchemaFrame frame{
///     sourcemeta::core::SchemaFrame::Mode::References, document,
///     sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver};
///
/// const sourcemeta::core::Pointer pointer{"items"};
/// const auto location{frame.traverse(
///     sourcemeta::core::to_weak_pointer(pointer),
///     sourcemeta::core::SchemaFrame::LocationType::Subschema)};
///
/// sourcemeta::core::WeakPointer base;
/// const sourcemeta::core::JSON result =
///   sourcemeta::blaze::wrap(document, frame, location.value().get(),
///     sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver,
///     base);
///
/// sourcemeta::core::prettify(result, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
auto wrap(const sourcemeta::core::JSON &schema,
          const sourcemeta::core::SchemaFrame &frame,
          const sourcemeta::core::SchemaFrame::Location &location,
          const sourcemeta::core::SchemaWalker &walker,
          const sourcemeta::core::SchemaResolver &resolver,
          sourcemeta::core::WeakPointer &base) -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
