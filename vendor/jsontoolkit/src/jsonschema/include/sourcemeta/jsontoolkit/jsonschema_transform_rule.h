#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_TRANSFORM_RULE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_TRANSFORM_RULE_H_

#include "jsonschema_export.h"

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema_transformer.h>

#include <optional> // std::optional, std::nullopt
#include <set>      // std::set
#include <string>   // std::string
#include <vector>   // std::vector

namespace sourcemeta::jsontoolkit {
/// @ingroup jsonschema
/// A class that represents a transformation rule to be used with
/// sourcemeta::jsontoolkit::SchemaTransformBundle. Clients of this class
/// are expected to subclass and implement their own condition and
/// transformation methods.
///
/// For example, this is a rule that deletes any property called `foo` in every
/// subschema:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// class MyRule final : public sourcemeta::jsontoolkit::SchemaTransformRule {
/// public:
///   MyRule() : sourcemeta::jsontoolkit::SchemaTransformRule("my_rule") {};
///
///   [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
///                                const std::string &dialect,
///                                const std::set<std::string> &vocabularies,
///                                const sourcemeta::jsontoolkit::Pointer
///                                  &pointer) const
///       -> bool override
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::jsontoolkit::SchemaTransformer &transformer)
///       const -> void override {
///     transformer.erase("foo");
///   }
/// };
/// ```
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaTransformRule {
public:
  /// Create a transformation rule. Each rule must have a unique name.
  SchemaTransformRule(std::string &&name,
                      std::string &&message = "Condition matched");

  // Necessary to wrap rules on smart pointers
  virtual ~SchemaTransformRule() = default;

  // We don't need any of these
  SchemaTransformRule(const SchemaTransformRule &) = delete;
  SchemaTransformRule(SchemaTransformRule &&) = delete;
  auto operator=(const SchemaTransformRule &) -> SchemaTransformRule & = delete;
  auto operator=(SchemaTransformRule &&) -> SchemaTransformRule & = delete;

  /// Compare a rule against another rule.
  auto operator==(const SchemaTransformRule &other) const -> bool;

  /// Fetch the name of a rule
  [[nodiscard]] auto name() const -> const std::string &;

  /// Fetch the message of a rule
  [[nodiscard]] auto message() const -> const std::string &;

  /// Apply the rule to a schema
  auto apply(JSON &schema, const Pointer &pointer,
             const SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> std::vector<SchemaTransformerOperation>;

  /// Check if the rule applies to a schema
  auto check(const JSON &schema, const Pointer &pointer,
             const SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> bool;

private:
  /// The rule condition
  [[nodiscard]] virtual auto
  condition(const JSON &schema, const std::string &dialect,
            const std::set<std::string> &vocabularies,
            const Pointer &pointer) const -> bool = 0;

  /// The rule transformation
  virtual auto transform(SchemaTransformer &transformer) const -> void = 0;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const std::string name_;
  const std::string message_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};
} // namespace sourcemeta::jsontoolkit

#endif
