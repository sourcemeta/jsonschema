#ifndef SOURCEMETA_BLAZE_LINTER_H_
#define SOURCEMETA_BLAZE_LINTER_H_

#ifndef SOURCEMETA_BLAZE_LINTER_EXPORT
#include <sourcemeta/blaze/linter_export.h>
#endif

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/core/jsonschema.h>

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

} // namespace sourcemeta::blaze

#endif
