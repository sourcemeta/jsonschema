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
      -> bool override;
  auto transform(sourcemeta::core::JSON &) const -> void override;

private:
  const Compiler compiler_;
};

} // namespace sourcemeta::blaze

#endif
