#ifndef SOURCEMETA_BLAZE_EVALUATOR_H_
#define SOURCEMETA_BLAZE_EVALUATOR_H_

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/blaze/evaluator_error.h>
#include <sourcemeta/blaze/evaluator_instruction.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/regex.h>

#include <algorithm>   // std::min, std::any_of, std::find
#include <cassert>     // assert
#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <ranges>      // std::ranges
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

/// @defgroup evaluator Evaluator
/// @brief A high-performance JSON Schema evaluator
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/evaluator.h>
/// ```

namespace sourcemeta::blaze {

/// @ingroup evaluator
/// Represents a compiled schema ready for execution
struct Template {
  bool dynamic;
  bool track;
  std::vector<Instructions> targets;
  std::vector<std::pair<std::size_t, std::size_t>> labels;
  std::vector<InstructionExtra> extra;
};

/// @ingroup evaluator
/// Parse a template from JSON
auto SOURCEMETA_BLAZE_EVALUATOR_EXPORT
from_json(const sourcemeta::core::JSON &json) -> std::optional<Template>;

/// @ingroup evaluator
/// Represents the state of an instruction evaluation
enum class EvaluationType : std::uint8_t { Pre, Post };

/// @ingroup evaluator
/// A callback of this type is invoked after evaluating any keyword. The
/// arguments go as follows:
///
/// - The stage at which the instruction in question is
/// - Whether the evaluation was successful or not (always true before
/// evaluation)
/// - The instruction that was just evaluated
/// - The extra data associated with the instruction
/// - The evaluation path
/// - The instance location
/// - The annotation result, if any (otherwise null)
///
/// You can use this callback mechanism to implement arbitrary output formats.
// TODO(C++23): Use std::move_only_function when available in libc++
using Callback = std::function<void(
    const EvaluationType, bool, const Instruction &, const InstructionExtra &,
    const sourcemeta::core::WeakPointer &,
    const sourcemeta::core::WeakPointer &, const sourcemeta::core::JSON &)>;

/// @ingroup evaluator
class SOURCEMETA_BLAZE_EVALUATOR_EXPORT Evaluator {
public:
  /// This function evaluates a schema compiler template, returning a boolean
  /// without error information. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/evaluator.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonschema.h>
  ///
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON schema =
  ///     sourcemeta::core::parse_json(R"JSON({
  ///   "$schema": "https://json-schema.org/draft/2020-12/schema",
  ///   "type": "string"
  /// })JSON");
  ///
  /// const auto schema_template{sourcemeta::blaze::compile(
  ///     schema, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver,
  ///     sourcemeta::core::default_schema_compiler)};
  ///
  /// sourcemeta::blaze::Evaluator evaluator;
  /// const sourcemeta::core::JSON instance{"foo bar"};
  /// const auto result{evaluator.validate(schema_template, instance)};
  /// assert(result);
  /// ```
  inline auto validate(const Template &schema,
                       const sourcemeta::core::JSON &instance) -> bool {
    assert(this->evaluate_path.empty());
    assert(this->instance_location.empty());
    assert(this->resources.empty());

    if (schema.track && schema.dynamic) [[unlikely]] {
      this->evaluated_.clear();
      return this->evaluate_impl<true, true, false>(schema, instance, nullptr);
    } else if (schema.track) [[unlikely]] {
      this->evaluated_.clear();
      return this->evaluate_impl<true, false, false>(schema, instance, nullptr);
    } else if (schema.dynamic) [[unlikely]] {
      return this->evaluate_impl<false, true, false>(schema, instance, nullptr);
    } else {
      return this->evaluate_impl<false, false, false>(schema, instance,
                                                      nullptr);
    }
  }

  /// This method evaluates a schema compiler template, executing the given
  /// callback at every step of the way. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/evaluator.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonschema.h>
  ///
  /// #include <cassert>
  /// #include <iostream>
  ///
  /// const sourcemeta::core::JSON schema =
  ///     sourcemeta::core::parse_json(R"JSON({
  ///   "$schema": "https://json-schema.org/draft/2020-12/schema",
  ///   "type": "string"
  /// })JSON");
  ///
  /// const auto schema_template{sourcemeta::blaze::compile(
  ///     schema, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver,
  ///     sourcemeta::core::default_schema_compiler)};
  ///
  /// static auto callback(
  ///     bool result,
  ///     const sourcemeta::blaze::Instruction &instruction,
  ///     const sourcemeta::core::Pointer &evaluate_path,
  ///     const sourcemeta::core::Pointer &instance_location,
  ///     const sourcemeta::core::JSON &document,
  ///     const sourcemeta::core::JSON &annotation) -> void {
  ///   std::cout << "TYPE: " << (result ? "Success" : "Failure") << "\n";
  ///   std::cout << "INSTRUCTION:\n";
  ///   sourcemeta::core::prettify(sourcemeta::blaze::to_json({instruction}),
  ///                                     std::cout);
  ///   std::cout << "\nEVALUATE PATH:";
  ///   sourcemeta::core::stringify(evaluate_path, std::cout);
  ///   std::cout << "\nINSTANCE LOCATION:";
  ///   sourcemeta::core::stringify(instance_location, std::cout);
  ///   std::cout << "\nANNOTATION:\n";
  ///   sourcemeta::core::prettify(annotation, std::cout);
  ///   std::cout << "\n";
  /// }
  ///
  /// sourcemeta::blaze::Evaluator evaluator;
  /// const sourcemeta::core::JSON instance{"foo bar"};
  /// const auto result{evaluator.validate(
  ///   schema_template, instance, callback)};
  ///
  /// assert(result);
  /// ```
  inline auto validate(const Template &schema,
                       const sourcemeta::core::JSON &instance,
                       const Callback &callback) -> bool {
    assert(this->evaluate_path.empty());
    assert(this->instance_location.empty());
    assert(this->resources.empty());
    this->evaluated_.clear();
    return this->evaluate_impl<true, true, true>(schema, instance, &callback);
  }

#ifndef DOXYGEN
  template <bool Track, bool Dynamic, bool HasCallback>
  auto evaluate_impl(const Template &schema,
                     const sourcemeta::core::JSON &instance,
                     const Callback *callback) -> bool;

  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
  static inline const sourcemeta::core::JSON null{nullptr};
  static inline const sourcemeta::core::JSON empty_string{""};
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

  // Compute a hash that fits within the IEEE 754 double-precision safe
  // integer range (2^53 - 1), ensuring the serialized template labels
  // are usable from JavaScript and other languages whose numbers are doubles
  [[nodiscard]] static auto hash(const std::size_t resource,
                                 const std::string_view fragment) noexcept
      -> std::size_t {
    constexpr std::size_t mask{(1ULL << 53) - 1};
    std::size_t result{14695981039346656037ULL & mask};
    for (const auto byte : fragment) {
      result ^= static_cast<std::size_t>(static_cast<unsigned char>(byte));
      result = (result * 1099511628211ULL) & mask;
    }

    return (resource + result) & mask;
  }

  auto evaluate(const sourcemeta::core::JSON *target) -> void {
    Evaluation mark{.instance = target,
                    .evaluate_path = this->evaluate_path,
                    .skip = false};
    this->evaluated_.push_back(std::move(mark));
  }

  [[nodiscard]] auto is_evaluated(const sourcemeta::core::JSON *target) const
      -> bool {
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto iterator = this->evaluated_.rbegin();
         iterator != this->evaluated_.rend(); ++iterator) {
      if (target == iterator->instance && !iterator->skip &&
          iterator->evaluate_path.starts_with_initial(this->evaluate_path)) {
        return true;
      }
    }

    return false;
  }

  auto unevaluate() -> void {
    for (auto &entry : this->evaluated_) {
      if (!entry.skip && entry.evaluate_path.starts_with(this->evaluate_path)) {
        entry.skip = true;
      }
    }
  }

#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  sourcemeta::core::WeakPointer evaluate_path;
  sourcemeta::core::WeakPointer instance_location;
  std::vector<std::size_t> resources;

  struct Evaluation {
    const sourcemeta::core::JSON *instance;
    sourcemeta::core::WeakPointer evaluate_path;
    bool skip;
  };

  std::vector<Evaluation> evaluated_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::blaze

#ifndef DOXYGEN

#include <sourcemeta/blaze/evaluator_dispatch.h>

template <bool Track, bool Dynamic, bool HasCallback>
auto sourcemeta::blaze::Evaluator::evaluate_impl(
    const Template &schema, const sourcemeta::core::JSON &instance,
    const Callback *callback) -> bool {
  assert(!schema.targets.empty());
  dispatch::DispatchContext<Track, Dynamic, HasCallback> context{
      .schema = &schema,
      .callback = callback,
      .evaluator = this,
      .property_target = nullptr};
  bool overall{true};
  for (const auto &instruction : schema.targets[0]) {
    if (!dispatch::evaluate_instruction(instruction, instance, 0, context))
        [[unlikely]] {
      overall = false;
      break;
    }
  }

  if constexpr (Track || HasCallback) {
    assert(this->evaluate_path.empty());
  }
  if constexpr (HasCallback) {
    assert(this->instance_location.empty());
  }
  if constexpr (Dynamic) {
    assert(this->resources.empty());
  }

  return overall;
}

#endif // !DOXYGEN

#endif
