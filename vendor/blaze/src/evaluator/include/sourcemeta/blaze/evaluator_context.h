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

  auto push(const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
            const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
            const std::size_t &schema_resource, const bool dynamic,
            const bool track) -> void;
  // A performance shortcut for pushing without re-traversing the target
  // if we already know that the destination target will be
  auto push(const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
            const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
            const std::size_t &schema_resource, const bool dynamic,
            const bool track,
            std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>
                &&new_instance) -> void;
  auto pop(const bool dynamic, const bool track) -> void;
  auto
  enter(const sourcemeta::jsontoolkit::WeakPointer::Token::Property &property,
        const bool track) -> void;
  auto enter(const sourcemeta::jsontoolkit::WeakPointer::Token::Index &index,
             const bool track) -> void;
  auto leave(const bool track) -> void;
  auto push_without_traverse(
      const sourcemeta::jsontoolkit::Pointer &relative_schema_location,
      const sourcemeta::jsontoolkit::Pointer &relative_instance_location,
      const std::size_t &schema_resource, const bool dynamic, const bool track)
      -> void;

  ///////////////////////////////////////////////
  // Target resolution
  ///////////////////////////////////////////////

  auto resolve_target() -> const sourcemeta::jsontoolkit::JSON &;
  auto resolve_string_target() -> std::optional<
      std::reference_wrapper<const sourcemeta::jsontoolkit::JSON::String>>;

  ///////////////////////////////////////////////
  // References and anchors
  ///////////////////////////////////////////////

  auto hash(const std::size_t &resource,
            const std::string &fragment) const noexcept -> std::size_t;

  ///////////////////////////////////////////////
  // Evaluation
  ///////////////////////////////////////////////

  auto evaluate() -> void;
  auto evaluate(const sourcemeta::jsontoolkit::WeakPointer::Token::Index from,
                const sourcemeta::jsontoolkit::WeakPointer::Token::Index to)
      -> void;
  auto
  evaluate(const sourcemeta::jsontoolkit::Pointer &relative_instance_location)
      -> void;
  auto
  is_evaluated(const sourcemeta::jsontoolkit::WeakPointer::Token &tail) const
      -> bool;
  auto unevaluate() -> void;

  // TODO: Remove this
  const sourcemeta::jsontoolkit::JSON null{nullptr};

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  std::vector<std::reference_wrapper<const sourcemeta::jsontoolkit::JSON>>
      instances;
  sourcemeta::jsontoolkit::WeakPointer evaluate_path;
  std::uint64_t evaluate_path_size{0};
  sourcemeta::jsontoolkit::WeakPointer instance_location;
  std::vector<std::pair<std::size_t, std::size_t>> frame_sizes;
  const std::hash<std::string> hasher_{};
  std::vector<std::size_t> resources;
  std::map<std::size_t, const std::reference_wrapper<const Template>> labels;
  std::optional<
      std::reference_wrapper<const sourcemeta::jsontoolkit::JSON::String>>
      property_target;

  // TODO: Revamp the data structure we use to track evaluation
  // to provide more performant lookups that don't involve so many
  // pointer token string comparisons
  struct Evaluation {
    sourcemeta::jsontoolkit::WeakPointer instance_location;
    sourcemeta::jsontoolkit::WeakPointer evaluate_path;
    bool skip;
  };

  std::vector<Evaluation> evaluated_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::blaze

#endif
