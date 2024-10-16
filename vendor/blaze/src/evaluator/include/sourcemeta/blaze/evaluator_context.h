#ifndef SOURCEMETA_BLAZE_EVALUATOR_CONTEXT_H
#define SOURCEMETA_BLAZE_EVALUATOR_CONTEXT_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include "evaluator_export.h"
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
  // Annotations
  ///////////////////////////////////////////////

  // TODO: At least currently, we only need to mask if a schema
  // makes use of `unevaluatedProperties` or `unevaluatedItems`
  // Detect if a schema does need this so if not, we avoid
  // an unnecessary copy
  auto mask() -> void;
  auto annotate(
      const sourcemeta::jsontoolkit::WeakPointer &current_instance_location,
      const sourcemeta::jsontoolkit::JSON &value)
      -> std::pair<std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>,
                   bool>;
  auto defines_any_annotation(const std::string &keyword) const -> bool;
  auto
  defines_sibling_annotation(const std::vector<std::string> &keywords,
                             const sourcemeta::jsontoolkit::JSON &value) const
      -> bool;
  auto largest_annotation_index(const std::string &keyword) const
      -> std::uint64_t;

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

  // For annotations
  std::vector<sourcemeta::jsontoolkit::WeakPointer> annotation_blacklist;
  std::map<sourcemeta::jsontoolkit::WeakPointer,
           std::map<sourcemeta::jsontoolkit::WeakPointer,
                    std::set<sourcemeta::jsontoolkit::JSON>>>
      annotations_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::blaze

#endif
