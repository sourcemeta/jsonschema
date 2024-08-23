#ifndef SOURCEMETA_ALTERSCHEMA_BUNDLE_H_
#define SOURCEMETA_ALTERSCHEMA_BUNDLE_H_

#include "engine_export.h"

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <sourcemeta/alterschema/engine_rule.h>

#include <cassert>     // assert
#include <concepts>    // std::derived_from
#include <functional>  // std::function
#include <map>         // std::map
#include <memory>      // std::make_unique, std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::alterschema {
/// @ingroup engine
///
/// You can use this class to perform top-down transformations on subschemas
/// given a set of rules. For example, we can remove every property `foo` as
/// follows:
///
/// ```cpp
/// #include <sourcemeta/alterschema/engine.h>
/// #include <cassert>
///
/// // Declare one or more rules
/// class MyRule final : public sourcemeta::alterschema::Rule {
/// public:
///   MyRule() : sourcemeta::alterschema::Rule("my_rule") {};
///
///   [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
///                                const std::string &dialect,
///                                const std::set<std::string> &vocabularies,
///                                const sourcemeta::jsontoolkit::Pointer
///                                  &pointer) const
///       -> bool override {
///     return schema.defines("foo");
///   }
///
///   auto transform(sourcemeta::alterschema::Transformer &transformer)
///       const -> void override {
///     transformer.erase("foo");
///   }
/// };
///
/// // Create a bundle
/// sourcemeta::alterschema::Bundle bundle;
///
/// // Register every rule
/// bundle.add<MyRule>();
///
/// // The input schema to transform
/// sourcemeta::jsontoolkit::JSON schema =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// // Apply the transformation bundle to the schema
/// bundle.apply(schema, sourcemeta::jsontoolkit::default_schema_walker,
///              sourcemeta::jsontoolkit::official_resolver);
///
/// // `foo` keywords are gone
/// assert(!schema.defines("foo"));
/// assert(!schema.at("items").defines("foo"));
/// ```
///
/// Every registered rule is applied to every subschema of the passed schema
/// until no longer of them applies.
class SOURCEMETA_ALTERSCHEMA_ENGINE_EXPORT Bundle {
public:
  /// Create a transform bundle
  Bundle() = default;

  // Not worth documenting these details
#if !defined(DOXYGEN)
  // Explicitly disallow copying, as this class makes use of unique pointers,
  // which by definition do not support copying. MSVC gets confused unless we
  // are explicit about it here.
  Bundle(const Bundle &) = delete;
  auto operator=(const Bundle &) -> Bundle & = delete;
  Bundle(Bundle &&) = default;
  auto operator=(Bundle &&) -> Bundle & = default;
#endif

  /// Add a rule to the bundle
  template <std::derived_from<Rule> T> auto add() -> void {
    auto rule{std::make_unique<T>()};
    // Rules must only be defined once
    assert(!this->rules.contains(rule->name()));
    this->rules.emplace(rule->name(), std::move(rule));
  }

  /// Apply the bundle of rules to a schema
  auto apply(sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::SchemaWalker &walker,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const sourcemeta::jsontoolkit::Pointer &pointer =
                 sourcemeta::jsontoolkit::empty_pointer,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> void;

  /// The callback that is called whenever the "check" functionality reports a
  /// rule whose condition holds true. The arguments are as follows:
  ///
  /// - The JSON Pointer to the given subschema
  /// - The name of the rule
  /// - The message of the rule
  using CheckCallback =
      std::function<void(const sourcemeta::jsontoolkit::Pointer &,
                         const std::string_view, const std::string_view)>;

  /// Report back the rules from the bundle that need to be applied to a schema
  auto check(const sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::SchemaWalker &walker,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const CheckCallback &callback,
             const sourcemeta::jsontoolkit::Pointer &pointer =
                 sourcemeta::jsontoolkit::empty_pointer,
             const std::optional<std::string> &default_dialect =
                 std::nullopt) const -> bool;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::map<std::string, std::unique_ptr<Rule>> rules;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};
} // namespace sourcemeta::alterschema

#endif
