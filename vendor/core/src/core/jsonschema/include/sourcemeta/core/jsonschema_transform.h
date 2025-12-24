#ifndef SOURCEMETA_CORE_JSONSCHEMA_RULE_H_
#define SOURCEMETA_CORE_JSONSCHEMA_RULE_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cassert>     // assert
#include <concepts>    // std::derived_from, std::same_as
#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <iterator>    // std::make_move_iterator, std::begin, std::end
#include <memory>      // std::make_unique, std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::forward, std::pair
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
///                                const sourcemeta::core::Vocabularies
///                                  &vocabularies,
///                                const sourcemeta::core::SchemaFrame &,
///                                const sourcemeta::core::SchemaFrame::Location
///                                &)
///       const -> sourcemeta::core::SchemaTransformRule::Result override
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::core::JSON &schema)
///       const -> void override {
///     schema.erase("foo");
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

  /// The result of evaluating a rule
  struct Result {
    Result(const bool applies_) : applies{applies_} {}
    Result(const Pointer &pointer) : applies{true}, locations{pointer} {
      assert(this->locations.size() == 1);
    }

    template <typename T>
      requires std::same_as<typename T::value_type, Pointer>
    Result(T &&container) : applies{true} {
      auto &&input = std::forward<T>(container);
      if constexpr (requires { input.size(); }) {
        locations.reserve(input.size());
      }

      locations.assign(std::make_move_iterator(std::begin(input)),
                       std::make_move_iterator(std::end(input)));
    }

    Result(std::vector<Pointer> &&locations_, JSON::String &&description_)
        : applies{true}, locations{std::move(locations_)},
          description{std::move(description_)} {}

    bool applies;
    std::vector<Pointer> locations;
    std::optional<JSON::String> description;
  };

  /// Apply the rule to a schema
  auto apply(JSON &schema, const JSON &root, const Vocabularies &vocabularies,
             const SchemaWalker &walker, const SchemaResolver &resolver,
             const SchemaFrame &frame,
             const SchemaFrame::Location &location) const
      -> std::pair<bool, Result>;

  /// Check if the rule applies to a schema
  [[nodiscard]] auto
  check(const JSON &schema, const JSON &root, const Vocabularies &vocabularies,
        const SchemaWalker &walker, const SchemaResolver &resolver,
        const SchemaFrame &frame, const SchemaFrame::Location &location) const
      -> Result;

  /// A method to optionally fix any reference location that was affected by the
  /// transformation.
  [[nodiscard]] virtual auto
  rereference(const std::string &reference, const Pointer &origin,
              const Pointer &target, const Pointer &current) const -> Pointer;

private:
  /// The rule condition
  [[nodiscard]] virtual auto
  condition(const JSON &schema, const JSON &root,
            const Vocabularies &vocabularies, const SchemaFrame &frame,
            const SchemaFrame::Location &location, const SchemaWalker &walker,
            const SchemaResolver &resolver) const -> Result = 0;

  /// The rule transformation. If this virtual method is not overriden,
  /// then the rule condition is considered to not be fixable.
  virtual auto transform(JSON &schema, const Result &result) const -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const std::string name_{};
  const std::string message_{};
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
///                                const sourcemeta::core::Vocabularies
///                                  &vocabularies,
///                                const sourcemeta::core::SchemaFrame &,
///                                const sourcemeta::core::SchemaFrame::Location
///                                &)
///       const -> sourcemeta::core::SchemaTransformRule::Result override {
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::core::JSON &schema)
///       const -> void override {
///     schema.erase("foo");
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
/// bundle.apply(schema, sourcemeta::core::schema_walker,
///              sourcemeta::core::schema_resolver);
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

  /// Add a rule to the bundle. Rules are evaluated in the order they are added.
  /// It is the caller's responsibility to not add duplicate rules.
  template <std::derived_from<SchemaTransformRule> T, typename... Args>
  auto add(Args &&...args) -> void {
    this->rules.push_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  /// Remove a rule from the bundle
  auto remove(const std::string &name) -> bool;

  /// The callback that is called whenever the condition of a rule holds true.
  /// The arguments are as follows:
  ///
  /// - The JSON Pointer to the given subschema
  /// - The name of the rule
  /// - The message of the rule
  /// - The rule evaluation result
  using Callback = std::function<void(const Pointer &, const std::string_view,
                                      const std::string_view,
                                      const SchemaTransformRule::Result &)>;

  /// Apply the bundle of rules to a schema
  [[nodiscard]] auto
  apply(JSON &schema, const SchemaWalker &walker,
        const SchemaResolver &resolver, const Callback &callback,
        const std::optional<JSON::String> &default_dialect = std::nullopt,
        const std::optional<JSON::String> &default_id = std::nullopt) const
      -> std::pair<bool, std::uint8_t>;

  /// Report back the rules from the bundle that need to be applied to a schema
  [[nodiscard]] auto
  check(const JSON &schema, const SchemaWalker &walker,
        const SchemaResolver &resolver, const Callback &callback,
        const std::optional<JSON::String> &default_dialect = std::nullopt,
        const std::optional<JSON::String> &default_id = std::nullopt) const
      -> std::pair<bool, std::uint8_t>;

  [[nodiscard]] auto begin() const -> auto { return this->rules.cbegin(); }
  [[nodiscard]] auto end() const -> auto { return this->rules.cend(); }

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<std::unique_ptr<SchemaTransformRule>> rules;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
