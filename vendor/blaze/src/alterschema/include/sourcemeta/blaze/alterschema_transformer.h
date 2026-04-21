#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_TRANSFORMER_H_
#define SOURCEMETA_BLAZE_ALTERSCHEMA_TRANSFORMER_H_

#ifndef SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT
#include <sourcemeta/blaze/alterschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

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
#include <tuple>       // std::tuple
#include <type_traits> // std::is_same_v, std::true_type
#include <utility>     // std::move, std::forward
#include <vector>      // std::vector

namespace sourcemeta::blaze {

/// @ingroup alterschema
///
/// A class that represents a transformation rule. Clients of this class
/// are expected to subclass and implement their own condition and
/// transformation methods.
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaTransformRule {
public:
  SchemaTransformRule(const std::string_view name,
                      const std::string_view message);

  virtual ~SchemaTransformRule() = default;

  SchemaTransformRule(const SchemaTransformRule &) = delete;
  SchemaTransformRule(SchemaTransformRule &&) = delete;
  auto operator=(const SchemaTransformRule &) -> SchemaTransformRule & = delete;
  auto operator=(SchemaTransformRule &&) -> SchemaTransformRule & = delete;

  /// Compare a rule against another rule
  auto operator==(const SchemaTransformRule &other) const -> bool;

  /// Fetch the name of a rule
  [[nodiscard]] auto name() const noexcept -> std::string_view;

  /// Fetch the message of a rule
  [[nodiscard]] auto message() const noexcept -> std::string_view;

  /// The result of evaluating a rule
  struct Result {
    Result(const bool applies_) : applies{applies_} {}
    Result(const sourcemeta::core::Pointer &pointer)
        : applies{true}, locations{pointer} {
      assert(this->locations.size() == 1);
    }

    template <typename T>
      requires std::same_as<typename T::value_type, sourcemeta::core::Pointer>
    Result(T &&container) : applies{true} {
      auto &&input = std::forward<T>(container);
      if constexpr (requires { input.size(); }) {
        locations.reserve(input.size());
      }

#if __cpp_lib_containers_ranges >= 202202L
      locations.assign_range(input);
#else
      locations.assign(std::make_move_iterator(std::begin(input)),
                       std::make_move_iterator(std::end(input)));
#endif
    }

    Result(std::vector<sourcemeta::core::Pointer> &&locations_,
           sourcemeta::core::JSON::String &&description_)
        : applies{true}, locations{std::move(locations_)},
          description{std::move(description_)} {}

    bool applies;
    std::vector<sourcemeta::core::Pointer> locations;
    std::optional<sourcemeta::core::JSON::String> description;
  };

  /// Check if the rule applies to a schema
  [[nodiscard]] auto
  check(const sourcemeta::core::JSON &schema,
        const sourcemeta::core::JSON &root,
        const sourcemeta::core::Vocabularies &vocabularies,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const sourcemeta::core::SchemaFrame &frame,
        const sourcemeta::core::SchemaFrame::Location &location,
        const sourcemeta::core::JSON::String &exclude_keyword) const -> Result;

  /// A method to optionally fix any reference location that was affected by the
  /// transformation
  [[nodiscard]] virtual auto
  rereference(const std::string_view reference,
              const sourcemeta::core::Pointer &origin,
              const sourcemeta::core::Pointer &target,
              const sourcemeta::core::Pointer &current) const
      -> sourcemeta::core::Pointer;

  /// The rule condition
  [[nodiscard]] virtual auto condition(
      const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
      const sourcemeta::core::Vocabularies &vocabularies,
      const sourcemeta::core::SchemaFrame &frame,
      const sourcemeta::core::SchemaFrame::Location &location,
      const sourcemeta::core::SchemaWalker &walker,
      const sourcemeta::core::SchemaResolver &resolver) const -> Result = 0;

  /// The rule transformation. If this virtual method is not overriden,
  /// then the rule is considered to not mutate the schema
  virtual auto transform(sourcemeta::core::JSON &schema,
                         const Result &result) const -> void;

private:
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

/// @ingroup alterschema
///
/// You can use this class to perform top-down transformations on subschemas
/// given a set of rules. Every registered rule is applied to every subschema
/// of the passed schema until no longer of them applies.
class SOURCEMETA_BLAZE_ALTERSCHEMA_EXPORT SchemaTransformer {
public:
  SchemaTransformer() = default;

#if !defined(DOXYGEN)
  SchemaTransformer(const SchemaTransformer &) = delete;
  auto operator=(const SchemaTransformer &) -> SchemaTransformer & = delete;
  SchemaTransformer(SchemaTransformer &&) = default;
  auto operator=(SchemaTransformer &&) -> SchemaTransformer & = default;
#endif

  /// Add a rule to the bundle. Rules are evaluated in the order they are added.
  /// It is the caller's responsibility to not add duplicate rules.
  template <std::derived_from<SchemaTransformRule> T, typename... Args>
  auto add(Args &&...args) -> std::string_view {
    static_assert(requires { typename T::mutates; });
    static_assert(requires { typename T::reframe_after_transform; });
    static_assert(
        std::is_same_v<typename T::mutates, std::true_type> ||
        std::is_same_v<typename T::reframe_after_transform, std::false_type>);
    auto &entry{this->rules.emplace_back(
        std::make_unique<T>(std::forward<Args>(args)...),
        std::is_same_v<typename T::mutates, std::true_type>,
        std::is_same_v<typename T::reframe_after_transform, std::true_type>)};
    return std::get<0>(entry)->name();
  }

  /// Remove a rule from the bundle
  auto remove(const std::string_view name) -> bool;

  /// The callback that is called whenever the condition of a rule holds true
  using Callback = std::function<void(
      const sourcemeta::core::Pointer &, const std::string_view,
      const std::string_view, const SchemaTransformRule::Result &, const bool)>;

  /// Apply the bundle of rules to a schema
  [[nodiscard]] auto
  apply(sourcemeta::core::JSON &schema,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const Callback &callback, std::string_view default_dialect = "",
        std::string_view default_id = "",
        const sourcemeta::core::JSON::String &exclude_keyword = "") const
      -> std::pair<bool, std::uint8_t>;

  /// Report back the rules from the bundle that need to be applied to a schema
  [[nodiscard]] auto
  check(const sourcemeta::core::JSON &schema,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::core::SchemaResolver &resolver,
        const Callback &callback, std::string_view default_dialect = "",
        std::string_view default_id = "",
        const sourcemeta::core::JSON::String &exclude_keyword = "") const
      -> std::pair<bool, std::uint8_t>;

  [[nodiscard]] auto begin() const -> auto { return this->rules.cbegin(); }
  [[nodiscard]] auto end() const -> auto { return this->rules.cend(); }

private:
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::vector<std::tuple<std::unique_ptr<SchemaTransformRule>, bool, bool>>
      rules;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
