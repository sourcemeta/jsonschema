#ifndef SOURCEMETA_ALTERSCHEMA_ENGINE_RULE_H_
#define SOURCEMETA_ALTERSCHEMA_ENGINE_RULE_H_

#include "engine_export.h"

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <sourcemeta/alterschema/engine_transformer.h>

#include <optional> // std::optional, std::nullopt
#include <set>      // std::set
#include <string>   // std::string
#include <vector>   // std::vector

namespace sourcemeta::alterschema {
/// @ingroup engine
///
/// A class that represents a transformation rule to be used with
/// sourcemeta::alterschema::Bundle. Clients of this class
/// are expected to subclass and implement their own condition and
/// transformation methods.
///
/// For example, this is a rule that deletes any property called `foo` in every
/// subschema:
///
/// ```cpp
/// #include <sourcemeta/alterschema/engine.h>
///
/// class MyRule final : public sourcemeta::alterschema::Rule {
/// public:
///   MyRule() : sourcemeta::alterschema::Rule("my_rule", "My rule") {};
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
///   auto transform(sourcemeta::alterschema::Transformer &transformer)
///       const -> void override {
///     transformer.erase("foo");
///   }
/// };
/// ```
class SOURCEMETA_ALTERSCHEMA_ENGINE_EXPORT Rule {
public:
  /// Create a transformation rule. Each rule must have a unique name.
  Rule(std::string &&name, std::string &&message);

  // Necessary to wrap rules on smart pointers
  virtual ~Rule() = default;

  // We don't need any of these
  Rule(const Rule &) = delete;
  Rule(Rule &&) = delete;
  auto operator=(const Rule &) -> Rule & = delete;
  auto operator=(Rule &&) -> Rule & = delete;

  /// Compare a rule against another rule.
  auto operator==(const Rule &other) const -> bool;

  /// Fetch the name of a rule
  [[nodiscard]] auto name() const -> const std::string &;

  /// Fetch the message of a rule
  [[nodiscard]] auto message() const -> const std::string &;

  /// Apply the rule to a schema
  auto apply(sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::Pointer &pointer,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> std::vector<Operation>;

  /// Check if the rule applies to a schema
  auto check(const sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::Pointer &pointer,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> bool;

private:
  /// The rule condition
  [[nodiscard]] virtual auto
  condition(const sourcemeta::jsontoolkit::JSON &schema,
            const std::string &dialect,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &pointer) const -> bool = 0;

  /// The rule transformation
  virtual auto transform(Transformer &transformer) const -> void = 0;

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
} // namespace sourcemeta::alterschema

#endif
