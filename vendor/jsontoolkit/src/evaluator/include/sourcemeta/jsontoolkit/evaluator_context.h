#ifndef SOURCEMETA_JSONTOOLKIT_EVALUATOR_CONTEXT_H
#define SOURCEMETA_JSONTOOLKIT_EVALUATOR_CONTEXT_H

#include "evaluator_export.h"

#include <sourcemeta/jsontoolkit/evaluator_template.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint8_t
#include <functional> // std::reference_wrapper
#include <map>        // std::map
#include <optional>   // std::optional
#include <set>        // std::set
#include <vector>     // std::vector

namespace sourcemeta::jsontoolkit {

/// @ingroup evaluator
/// Represents a stateful schema evaluation context
class SOURCEMETA_JSONTOOLKIT_EVALUATOR_EXPORT EvaluationContext {
public:
  /// Prepare the schema evaluation context with a given instance.
  /// Performing evaluation on a context without preparing it with
  /// an instance is undefined behavior.
  auto prepare(const JSON &instance) -> void;

  // All of these methods are considered internal and no
  // client must depend on them
#ifndef DOXYGEN

  ///////////////////////////////////////////////
  // Evaluation stack
  ///////////////////////////////////////////////

  auto evaluate_path() const noexcept -> const WeakPointer &;
  auto instance_location() const noexcept -> const WeakPointer &;
  auto push(const Pointer &relative_schema_location,
            const Pointer &relative_instance_location,
            const std::string &schema_resource, const bool dynamic) -> void;
  // A performance shortcut for pushing without re-traversing the target
  // if we already know that the destination target will be
  auto push(const Pointer &relative_schema_location,
            const Pointer &relative_instance_location,
            const std::string &schema_resource, const bool dynamic,
            std::reference_wrapper<const JSON> &&new_instance) -> void;
  auto pop(const bool dynamic) -> void;
  auto enter(const WeakPointer::Token::Property &property) -> void;
  auto enter(const WeakPointer::Token::Index &index) -> void;
  auto leave() -> void;

private:
  auto push_without_traverse(const Pointer &relative_schema_location,
                             const Pointer &relative_instance_location,
                             const std::string &schema_resource,
                             const bool dynamic) -> void;

public:
  ///////////////////////////////////////////////
  // Target resolution
  ///////////////////////////////////////////////

  auto instances() const noexcept
      -> const std::vector<std::reference_wrapper<const JSON>> &;
  enum class TargetType : std::uint8_t { Key, Value };
  auto target_type(const TargetType type) noexcept -> void;
  auto resolve_target() -> const JSON &;

  ///////////////////////////////////////////////
  // References and anchors
  ///////////////////////////////////////////////

  auto resources() const noexcept -> const std::vector<std::string> &;
  auto mark(const std::size_t id, const SchemaCompilerTemplate &children)
      -> void;
  // TODO: At least currently, we only need to mask if a schema
  // makes use of `unevaluatedProperties` or `unevaluatedItems`
  // Detect if a schema does need this so if not, we avoid
  // an unnecessary copy
  auto mask() -> void;
  auto jump(const std::size_t id) const noexcept
      -> const SchemaCompilerTemplate &;
  auto find_dynamic_anchor(const std::string &anchor) const
      -> std::optional<std::size_t>;

  ///////////////////////////////////////////////
  // Annotations
  ///////////////////////////////////////////////

  auto annotate(const WeakPointer &current_instance_location, const JSON &value)
      -> std::pair<std::reference_wrapper<const JSON>, bool>;
  auto
  defines_any_adjacent_annotation(const WeakPointer &expected_instance_location,
                                  const WeakPointer &base_evaluate_path,
                                  const std::string &keyword) const -> bool;
  auto defines_any_adjacent_annotation(
      const WeakPointer &expected_instance_location,
      const WeakPointer &base_evaluate_path,
      const std::vector<std::string> &keywords) const -> bool;
  auto defines_annotation(const WeakPointer &expected_instance_location,
                          const WeakPointer &base_evaluate_path,
                          const std::vector<std::string> &keywords,
                          const JSON &value) const -> bool;
  auto largest_annotation_index(const WeakPointer &expected_instance_location,
                                const std::vector<std::string> &keywords,
                                const std::uint64_t default_value) const
      -> std::uint64_t;

private:
  auto annotations(const WeakPointer &current_instance_location,
                   const WeakPointer &schema_location) const
      -> const std::set<JSON> &;
  auto annotations(const WeakPointer &current_instance_location) const
      -> const std::map<WeakPointer, std::set<JSON>> &;

public:
  // TODO: Remove this
  const JSON null{nullptr};

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  std::vector<std::reference_wrapper<const JSON>> instances_;
  WeakPointer evaluate_path_;
  WeakPointer instance_location_;
  std::vector<std::pair<std::size_t, std::size_t>> frame_sizes;
  // TODO: Keep hashes of schema resources URI instead for performance reasons
  std::vector<std::string> resources_;
  std::vector<WeakPointer> annotation_blacklist;
  // We don't use a pair for holding the two pointers for runtime
  // efficiency when resolving keywords like `unevaluatedProperties`
  std::map<WeakPointer, std::map<WeakPointer, std::set<JSON>>> annotations_;
  std::map<std::size_t,
           const std::reference_wrapper<const SchemaCompilerTemplate>>
      labels;
  bool property_as_instance{false};
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::jsontoolkit

#endif
