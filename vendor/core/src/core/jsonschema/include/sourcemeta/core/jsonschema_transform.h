#ifndef SOURCEMETA_CORE_JSONSCHEMA_RULE_H_
#define SOURCEMETA_CORE_JSONSCHEMA_RULE_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema_resolver.h>

#include <cassert>     // assert
#include <concepts>    // std::derived_from
#include <functional>  // std::function
#include <map>         // std::map
#include <memory>      // std::make_unique, std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup jsonschema
///
/// A class that represents a transformation rule. Clients of this class
/// are expected to subclass and implement their own condition and
/// transformation methods.
///
/// For example, this is a rule that deletes any property called `foo` in every
/// subschema:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
///
/// class MySchemaTransformRule final
///   : public sourcemeta::core::SchemaTransformRule {
/// public:
///   MySchemaTransformRule() :
///   sourcemeta::core::SchemaTransformRule("my_rule", "My rule") {};
///
///   [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
///                                const std::string &dialect,
///                                const std::set<std::string> &vocabularies,
///                                const sourcemeta::core::Pointer
///                                  &pointer) const
///       -> bool override
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::core::PointerProxy &transformer)
///       const -> void override {
///     transformer.erase("foo");
///   }
/// };
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaTransformRule {
public:
  /// Create a transformation rule. Each rule must have a unique name.
  SchemaTransformRule(std::string &&name, std::string &&message);

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
  auto
  apply(JSON &schema, const Pointer &pointer, const SchemaResolver &resolver,
        const std::optional<std::string> &default_dialect = std::nullopt) const
      -> std::vector<PointerProxy::Operation>;

  /// Check if the rule applies to a schema
  auto
  check(const JSON &schema, const Pointer &pointer,
        const SchemaResolver &resolver,
        const std::optional<std::string> &default_dialect = std::nullopt) const
      -> bool;

private:
  /// The rule condition
  [[nodiscard]] virtual auto
  condition(const JSON &schema, const std::string &dialect,
            const std::set<std::string> &vocabularies,
            const Pointer &pointer) const -> bool = 0;

  /// The rule transformation
  virtual auto transform(PointerProxy &transformer) const -> void = 0;

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

/// @ingroup jsonschema
///
/// You can use this class to perform top-down transformations on subschemas
/// given a set of rules. For example, we can remove every property `foo` as
/// follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// // Declare one or more rules
/// class MyRule final
///   : public sourcemeta::core::SchemaTransformRule {
/// public:
///   MyRule() : sourcemeta::core::SchemaTransformRule("my_rule") {};
///
///   [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
///                                const std::string &dialect,
///                                const std::set<std::string> &vocabularies,
///                                const sourcemeta::core::Pointer
///                                  &pointer) const
///       -> bool override {
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::core::PointerProxy &transformer)
///       const -> void override {
///     transformer.erase("foo");
///   }
/// };
///
/// // Create a bundle
/// sourcemeta::core::SchemaTransformer bundle;
///
/// // Register every rule
/// bundle.add<MyRule>();
///
/// // The input schema to transform
/// sourcemeta::core::JSON schema =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// // Apply the transformation bundle to the schema
/// bundle.apply(schema, sourcemeta::core::schema_official_walker,
///              sourcemeta::core::schema_official_resolver);
///
/// // `foo` keywords are gone
/// assert(!schema.defines("foo"));
/// assert(!schema.at("items").defines("foo"));
/// ```
///
/// Every registered rule is applied to every subschema of the passed schema
/// until no longer of them applies.
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaTransformer {
public:
  /// Create a transform bundle
  SchemaTransformer() = default;

  // Not worth documenting these details
#if !defined(DOXYGEN)
  // Explicitly disallow copying, as this class makes use of unique pointers,
  // which by definition do not support copying. MSVC gets confused unless we
  // are explicit about it here.
  SchemaTransformer(const SchemaTransformer &) = delete;
  auto operator=(const SchemaTransformer &) -> SchemaTransformer & = delete;
  SchemaTransformer(SchemaTransformer &&) = default;
  auto operator=(SchemaTransformer &&) -> SchemaTransformer & = default;
#endif

  /// Add a rule to the bundle
  template <std::derived_from<SchemaTransformRule> T> auto add() -> void {
    auto rule{std::make_unique<T>()};
    // Rules must only be defined once
    assert(!this->rules.contains(rule->name()));
    this->rules.emplace(rule->name(), std::move(rule));
  }

  /// Apply the bundle of rules to a schema
  auto
  apply(JSON &schema, const SchemaWalker &walker,
        const SchemaResolver &resolver, const Pointer &pointer = empty_pointer,
        const std::optional<std::string> &default_dialect = std::nullopt) const
      -> void;

  /// The callback that is called whenever the "check" functionality reports a
  /// rule whose condition holds true. The arguments are as follows:
  ///
  /// - The JSON Pointer to the given subschema
  /// - The name of the rule
  /// - The message of the rule
  using CheckCallback = std::function<void(
      const Pointer &, const std::string_view, const std::string_view)>;

  /// Report back the rules from the bundle that need to be applied to a schema
  auto
  check(const JSON &schema, const SchemaWalker &walker,
        const SchemaResolver &resolver, const CheckCallback &callback,
        const Pointer &pointer = empty_pointer,
        const std::optional<std::string> &default_dialect = std::nullopt) const
      -> bool;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::map<std::string, std::unique_ptr<SchemaTransformRule>> rules;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
