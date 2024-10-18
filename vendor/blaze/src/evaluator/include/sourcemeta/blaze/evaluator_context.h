#ifndef SOURCEMETA_BLAZE_EVALUATOR_CONTEXT_H
#define SOURCEMETA_BLAZE_EVALUATOR_CONTEXT_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/blaze/evaluator_template.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint8_t
#include <functional> // std::reference_wrapper
#include <map>        // std::map
#include <optional>   // std::optional
#include <set>        // std::set
#include <string>     // std::string
#include <vector>     // std::vector

namespace sourcemeta::blaze {

/// @ingroup evaluator
/// Represents a stateful schema evaluation context
class SOURCEMETA_BLAZE_EVALUATOR_EXPORT EvaluationContext {
public:
  /// Prepare the schema evaluation context with a given instance.
  /// Performing evaluation on a context without preparing it with
  /// an instance is undefined behavior.
  auto prepare(const sourcemeta::jsontoolkit::JSON &instance) -> void;

  // All of these methods are considered internal and no
  // client must depend on them
#ifndef DOXYGEN

  ///////////////////////////////////////////////
  // Evaluation stack
  ///////////////////////////////////////////////

  auto evaluate_path() const noexcept
      -> const sourcemeta::jsontoolkit::WeakPointer &;
  auto instance_location() const noexcept
      -> const sourcemeta::jsontoolkit::WeakPointer &;
  auto push(const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
            const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
            const std::size_t &schema_resource, const bool dynamic) -> void;
  // A performance shortcut for pushing without re-traversing the target
  // if we already know that the destination target will be
  auto push(const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
            const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
            const std::size_t &schema_resource, const bool dynamic,
            std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>
                &&new_instance) -> void;
  auto pop(const bool dynamic) -> void;
  auto
  enter(const sourcemeta::jsontoolkit::WeakPointer::Token::Property &property)
      -> void;
  auto enter(const sourcemeta::jsontoolkit::WeakPointer::Token::Index &index)
      -> void;
  auto leave() -> void;

private:
  auto push_without_traverse(
      const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
      const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
      const std::size_t &schema_resource, const bool dynamic) -> void;

public:
  ///////////////////////////////////////////////
  // Target resolution
  ///////////////////////////////////////////////

  auto instances() const noexcept -> const std::vector<
      std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>> &;
  enum class TargetType : std::uint8_t { Key, Value };
  auto target_type(const TargetType type) noexcept -> void;
  auto resolve_target() -> const sourcemeta::jsontoolkit::JSON &;
  auto resolve_string_target() -> std::optional<
      std::reference_wrapper<const sourcemeta::jsontoolkit::JSON::String>>;

  ///////////////////////////////////////////////
  // References and anchors
  ///////////////////////////////////////////////

  auto hash(const std::size_t &resource,
            const std::string &fragment) const noexcept -> std::size_t;
  auto resources() const noexcept -> const std::vector<std::size_t> &;
  auto mark(const std::size_t id, const Template &children) -> void;
  auto jump(const std::size_t id) const noexcept -> const Template &;
  auto find_dynamic_anchor(const std::string &anchor) const
      -> std::optional<std::size_t>;

  ///////////////////////////////////////////////
  // Evaluation
  ///////////////////////////////////////////////

  auto evaluate() -> void;
  auto
  evaluate(const sourcemeta::jsontoolkit::Pointer &relative_instance_location)
      -> void;
  auto is_evaluated(sourcemeta::jsontoolkit::WeakPointer::Token &&token) const
      -> bool;
  auto unevaluate() -> void;

public:
  // TODO: Remove this
  const sourcemeta::jsontoolkit::JSON null{nullptr};

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  std::vector<std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>>
      instances_;
  sourcemeta::jsontoolkit::WeakPointer evaluate_path_;
  sourcemeta::jsontoolkit::WeakPointer instance_location_;
  std::vector<std::pair<std::size_t, std::size_t>> frame_sizes;
  const std::hash<std::string> hasher_{};
  std::vector<std::size_t> resources_;
  std::map<std::size_t, const std::reference_wrapper<const Template>> labels;
  bool property_as_instance{false};

  // TODO: Turn these into a trie
  std::vector<std::pair<sourcemeta::jsontoolkit::WeakPointer,
                        sourcemeta::jsontoolkit::WeakPointer>>
      evaluated_;
  std::vector<sourcemeta::jsontoolkit::WeakPointer> evaluated_blacklist_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::blaze

#endif
