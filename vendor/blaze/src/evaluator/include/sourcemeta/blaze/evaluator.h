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

#include <cassert>    // assert
#include <cstdint>    // std::uint8_t
#include <functional> // std::function
#include <ranges>     // std::views::reverse
#include <utility>    // std::pair
#include <vector>     // std::vector

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
  bool frames;
  std::vector<Instructions> targets;
  /// What every schema resource that declares at least one dynamic anchor binds
  /// on entry, as pairs of an index into Template::anchors and a one-based
  /// index into Template::targets. The first entry is always empty, so that
  /// zero stands for a resource that binds nothing
  std::vector<std::vector<std::pair<std::size_t, std::size_t>>> bindings;
  /// What each target binds on entry, as an index into Template::bindings
  std::vector<std::size_t> target_bindings;
  std::vector<InstructionExtra> extra;
  /// The dynamic anchor names that instructions refer to, without duplicates
  std::vector<std::string> anchors;
  /// The vocabulary URIs that instructions refer to, without duplicates
  std::vector<std::string> vocabularies;
  /// The message templates that instructions refer to, without duplicates
  std::vector<std::string> messages;
};

/// @ingroup evaluator
constexpr std::size_t JSON_VERSION{20};

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
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// sourcemeta::blaze::Evaluator evaluator;
  /// const sourcemeta::core::JSON instance{"foo bar"};
  /// const auto result{evaluator.validate(schema_template, instance)};
  /// assert(result);
  /// ```
  auto validate(const Template &schema, const sourcemeta::core::JSON &instance)
      -> bool {
    assert(this->evaluate_path.empty());
    assert(this->instance_location.empty());
    assert(this->binding_undo.empty());

    this->evaluated.clear();
    return this->evaluate_impl<false>(schema, instance, nullptr);
  }

  /// This method evaluates a schema compiler template, executing the given
  /// callback at every step of the way. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/evaluator.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <sourcemeta/core/jsonschema.h>
  ///
  /// #include <cassert>
  /// #include <cstddef>
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
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// static auto callback(
  ///     const sourcemeta::blaze::EvaluationType type,
  ///     const bool result,
  ///     const sourcemeta::blaze::Instruction &instruction,
  ///     const sourcemeta::blaze::InstructionExtra &extra,
  ///     const sourcemeta::core::WeakPointer &evaluate_path,
  ///     const sourcemeta::core::WeakPointer &instance_location,
  ///     const sourcemeta::core::JSON &annotation) -> void {
  ///   if (type == sourcemeta::blaze::EvaluationType::Pre) {
  ///     return;
  ///   }
  ///
  ///   std::cout << "RESULT: " << (result ? "Success" : "Failure") << "\n";
  ///   std::cout << "INSTRUCTION: "
  ///             << sourcemeta::blaze::InstructionNames[
  ///                  static_cast<std::size_t>(instruction.type)]
  ///             << "\n";
  ///   std::cout << "KEYWORD LOCATION: " << extra.keyword_location << "\n";
  ///   std::cout << "EVALUATE PATH: ";
  ///   sourcemeta::core::stringify(evaluate_path, std::cout);
  ///   std::cout << "\nINSTANCE LOCATION: ";
  ///   sourcemeta::core::stringify(instance_location, std::cout);
  ///   std::cout << "\nANNOTATION: ";
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
  auto validate(const Template &schema, const sourcemeta::core::JSON &instance,
                const Callback &callback) -> bool {
    assert(this->evaluate_path.empty());
    assert(this->instance_location.empty());
    assert(this->binding_undo.empty());
    this->evaluated.clear();
    return this->evaluate_impl<true>(schema, instance, &callback);
  }

#ifndef DOXYGEN
  template <bool HasCallback>
  auto evaluate_impl(const Template &schema,
                     const sourcemeta::core::JSON &instance,
                     const Callback *callback) -> bool;

  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const sourcemeta::core::JSON NULL_VALUE{nullptr};
  static inline const sourcemeta::core::JSON EMPTY_STRING{""};
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp,bugprone-throwing-static-initialization)

  auto evaluate(const sourcemeta::core::JSON *target) -> void {
    this->evaluated.push_back(target);
  }

  [[nodiscard]] auto is_evaluated(const sourcemeta::core::JSON *target,
                                  const std::size_t frame) const -> bool {
    assert(frame <= this->evaluated.size());
    for (auto index = this->evaluated.size(); index > frame; index--) {
      if (this->evaluated[index - 1] == target) {
        return true;
      }
    }

    return false;
  }

  // A subschema that ends up failing must not contribute any of the marks it
  // made along the way, so a disjunction records the length here before each
  // branch and truncates back to it when the branch does not hold, as does a
  // negation for its whole subtree. Marks are only ever appended, never
  // inserted earlier, so everything a branch adds sits past the recorded
  // length and nothing from outside it can be lost
  [[nodiscard]] auto checkpoint() const -> std::size_t {
    return this->evaluated.size();
  }

  auto rewind(const std::size_t checkpoint) -> void {
    assert(checkpoint <= this->evaluated.size());
    this->evaluated.resize(checkpoint);
  }

  // What the schema resource the entry point sits in declares, which no
  // instruction stands for. This lives in a translation unit of its own, as
  // inlining it pushes the evaluation path past what its caller will take on,
  // which costs a call per evaluation on every schema, dynamic anchors or not
  auto enter_entrypoint_resource(const Template &schema) -> void;

  // A schema resource claims each dynamic anchor it declares that no enclosing
  // resource already holds, so a name resolves to its outermost declaration.
  // What the slots held goes on a stack of its own, as only a resource that
  // declares an anchor ever reaches here
  auto enter_resource(
      const std::vector<std::pair<std::size_t, std::size_t>> &bindings)
      -> void {
    for (const auto &binding : bindings) {
      assert(binding.first < this->dynamic_bindings.size());
      auto &slot{this->dynamic_bindings[binding.first]};
      this->binding_undo.push_back(slot);
      if (slot == 0) {
        slot = binding.second;
      }
    }
  }

  auto leave_resource(
      const std::vector<std::pair<std::size_t, std::size_t>> &bindings)
      -> void {
    for (const auto &binding : std::views::reverse(bindings)) {
      assert(!this->binding_undo.empty());
      assert(binding.first < this->dynamic_bindings.size());
      this->dynamic_bindings[binding.first] = this->binding_undo.back();
      this->binding_undo.pop_back();
    }
  }

  // What a dynamic reference resolves to, given the name it carries and the
  // static target it fell back on
  [[nodiscard]] auto dynamic_target(const std::size_t anchor,
                                    const std::size_t fallback) const
      -> std::size_t {
    assert(anchor < this->dynamic_bindings.size());
    const auto slot{this->dynamic_bindings[anchor]};
    return slot == 0 ? fallback : slot;
  }

#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  sourcemeta::core::WeakPointer evaluate_path;
  sourcemeta::core::WeakPointer instance_location;
  // What dynamic anchor each name currently resolves to, as a one-based index
  // into Template::targets where zero means the name is not in scope. A
  // resource writes a name on entry only when it is not already taken, so a
  // slot holds the outermost declaration in the dynamic scope
  std::vector<std::size_t> dynamic_bindings;
  // What the slots a resource wrote on entry held beforehand, so that leaving
  // it puts them back. Only a resource that declares a dynamic anchor touches
  // this, rather than every instruction
  std::vector<std::size_t> binding_undo;

  std::vector<const sourcemeta::core::JSON *> evaluated;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
#endif
};

} // namespace sourcemeta::blaze

#ifndef DOXYGEN

#include <sourcemeta/blaze/evaluator_dispatch.h>

template <bool HasCallback>
auto sourcemeta::blaze::Evaluator::evaluate_impl(
    const Template &schema, const sourcemeta::core::JSON &instance,
    const Callback *callback) -> bool {
  assert(!schema.targets.empty());
  assert(schema.target_bindings.size() == schema.targets.size());
  dispatch::DispatchContext<HasCallback> context{.schema = &schema,
                                                 .callback = callback,
                                                 .evaluator = this,
                                                 .property_target = nullptr,
                                                 .frame = 0};

  if (!schema.anchors.empty()) [[unlikely]] {
    this->enter_entrypoint_resource(schema);
  }

  bool overall{true};
  for (const auto &instruction : schema.targets[0]) {
    if (!dispatch::evaluate_instruction(instruction, instance, 0, context))
        [[unlikely]] {
      overall = false;
      break;
    }
  }

  if constexpr (HasCallback) {
    assert(this->evaluate_path.empty());
    assert(this->instance_location.empty());
  }

  assert(this->binding_undo.empty());

  return overall;
}

#endif // !DOXYGEN

#endif
