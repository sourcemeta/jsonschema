#ifndef SOURCEMETA_BLAZE_OUTPUT_SIMPLE_H_
#define SOURCEMETA_BLAZE_OUTPUT_SIMPLE_H_

#ifndef SOURCEMETA_BLAZE_OUTPUT_EXPORT
#include <sourcemeta/blaze/output_export.h>
#endif

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>

#include <functional> // std::reference_wrapper
// TODO(C++23): Consider std::flat_map/std::flat_set when available in libc++
#include <map>     // std::map
#include <ostream> // std::ostream
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

namespace sourcemeta::blaze {

/// @ingroup output
///
/// A simple evaluation callback that reports a stack trace in the case of
/// validation error that you can report as you with. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
/// #include <sourcemeta/blaze/output.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// #include <cassert>
/// #include <functional>
///
/// const sourcemeta::core::JSON schema =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::blaze::schema_walker,
///     sourcemeta::blaze::schema_resolver,
///     sourcemeta::blaze::default_schema_compiler)};
///
/// const sourcemeta::core::JSON instance{5};
///
/// sourcemeta::blaze::SimpleOutput output{instance};
/// sourcemeta::blaze::Evaluator evaluator;
/// const auto result{evaluator.validate(
///   schema_template, instance, std::ref(output))};
///
/// if (!result) {
///   for (const auto &entry : output) {
///     std::cerr << entry.message << "\n";
///     sourcemeta::core::stringify(entry.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::core::stringify(entry.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_BLAZE_OUTPUT_EXPORT SimpleOutput {
public:
  SimpleOutput(const sourcemeta::core::JSON &instance,
               sourcemeta::core::WeakPointer base =
                   sourcemeta::core::empty_weak_pointer);

  // Prevent accidental copies
  SimpleOutput(const SimpleOutput &) = delete;
  auto operator=(const SimpleOutput &) -> SimpleOutput & = delete;

  struct Entry {
    std::string message;
    sourcemeta::core::WeakPointer instance_location;
    sourcemeta::core::WeakPointer evaluate_path;
    std::reference_wrapper<const std::string> schema_location;
  };

  /// A single collected annotation, in evaluation order
  struct AnnotationEntry {
    sourcemeta::core::WeakPointer instance_location;
    sourcemeta::core::WeakPointer evaluate_path;
    std::reference_wrapper<const std::string> schema_location;
    sourcemeta::core::JSON value;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const InstructionExtra &step_metadata,
                  const sourcemeta::core::WeakPointer &evaluate_path,
                  const sourcemeta::core::WeakPointer &instance_location,
                  const sourcemeta::core::JSON &annotation) -> void;

  using container_type = typename std::vector<Entry>;
  using const_iterator = typename container_type::const_iterator;
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;
  [[nodiscard]] auto cbegin() const -> const_iterator;
  [[nodiscard]] auto cend() const -> const_iterator;

  /// Move out the collected error entries, leaving this output empty. Useful to
  /// take ownership of the trace without copying when the output is no longer
  /// needed
  [[nodiscard]] auto release() && -> container_type {
    auto result{std::move(this->output)};
    // A moved-from container is left in a valid but unspecified state, so
    // clear it to honor the documented empty postcondition portably
    this->output.clear();
    return result;
  }

  /// Access the annotations collected during evaluation, as a flat log in
  /// evaluation order. The log records every emission as-is, so a location may
  /// repeat and hold the same value more than once. Consumers that need them
  /// grouped by location, or collapsed to distinct values, index this log
  /// themselves
  [[nodiscard]] auto annotations() const -> const auto & {
    return this->annotations_;
  }

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const sourcemeta::core::JSON &instance_;
  const sourcemeta::core::WeakPointer base_;
  container_type output;
  std::vector<
      std::pair<sourcemeta::core::WeakPointer, sourcemeta::core::WeakPointer>>
      mask;
  std::map<
      std::pair<sourcemeta::core::WeakPointer, sourcemeta::core::WeakPointer>,
      std::vector<Entry>>
      masked_traces;
  std::vector<AnnotationEntry> annotations_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
}; // namespace sourcemeta::blaze

} // namespace sourcemeta::blaze

#endif
