#ifndef SOURCEMETA_BLAZE_LINTER_H_
#define SOURCEMETA_BLAZE_LINTER_H_

#ifndef SOURCEMETA_BLAZE_LINTER_EXPORT
#include <sourcemeta/blaze/linter_export.h>
#endif

#include <sourcemeta/blaze/linter_error.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/jsonschema.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <type_traits> // std::true_type, std::false_type

/// @defgroup linter Linter
/// @brief A set of JSON Schema linter extensions powered by Blaze
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/linter.h>
/// ```

namespace sourcemeta::blaze {

/// @ingroup linter
///
/// Check that the `examples` keyword consists of instances that match the
/// corresponding schema.
class SOURCEMETA_BLAZE_LINTER_EXPORT ValidExamples final
    : public sourcemeta::core::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ValidExamples(Compiler compiler);
  [[nodiscard]] auto condition(const sourcemeta::core::JSON &,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::Vocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override;
  auto transform(sourcemeta::core::JSON &,
                 const sourcemeta::core::SchemaTransformRule::Result &) const
      -> void override;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const Compiler compiler_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup linter
///
/// Check that the `default` keyword consists of an instance that matches the
/// corresponding schema.
class SOURCEMETA_BLAZE_LINTER_EXPORT ValidDefault final
    : public sourcemeta::core::SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ValidDefault(Compiler compiler);
  [[nodiscard]] auto condition(const sourcemeta::core::JSON &,
                               const sourcemeta::core::JSON &,
                               const sourcemeta::core::Vocabularies &,
                               const sourcemeta::core::SchemaFrame &,
                               const sourcemeta::core::SchemaFrame::Location &,
                               const sourcemeta::core::SchemaWalker &,
                               const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override;
  auto transform(sourcemeta::core::JSON &,
                 const sourcemeta::core::SchemaTransformRule::Result &) const
      -> void override;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const Compiler compiler_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup linter
///
/// A linter rule driven by a JSON Schema. Every subschema in the document
/// under inspection is validated as a JSON instance against the provided
/// rule schema. When a subschema does not conform, the rule fires and
/// reports the validation errors. The rule name is extracted from the
/// `title` keyword of the rule schema, and the rule description from the
/// `description` keyword. The title must consist only of lowercase ASCII
/// letters, digits, underscores, or slashes.
class SOURCEMETA_BLAZE_LINTER_EXPORT SchemaRule final
    : public sourcemeta::core::SchemaTransformRule {
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
      -> sourcemeta::core::SchemaTransformRule::Result override;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  Template template_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
