#ifndef SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_
#define SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>

#include <map>         // std::map
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::tie
#include <vector>      // std::vector

namespace sourcemeta::blaze {

/// @ingroup compiler
///
/// A simple evaluation callback that reports a stack trace in the case of
/// validation error that you can report as you with. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
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
///     schema, sourcemeta::core::schema_official_walker,
///     sourcemeta::core::schema_official_resolver,
///     sourcemeta::core::default_schema_compiler)};
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
class SOURCEMETA_BLAZE_COMPILER_EXPORT SimpleOutput {
public:
  SimpleOutput(const sourcemeta::core::JSON &instance,
               const sourcemeta::core::WeakPointer &base =
                   sourcemeta::core::empty_weak_pointer);

  // Prevent accidental copies
  SimpleOutput(const SimpleOutput &) = delete;
  auto operator=(const SimpleOutput &) -> SimpleOutput & = delete;

  struct Entry {
    const std::string message;
    const sourcemeta::core::WeakPointer instance_location;
    const sourcemeta::core::WeakPointer evaluate_path;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const sourcemeta::core::WeakPointer &evaluate_path,
                  const sourcemeta::core::WeakPointer &instance_location,
                  const sourcemeta::core::JSON &annotation) -> void;

  using container_type = typename std::vector<Entry>;
  using const_iterator = typename container_type::const_iterator;
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

  /// Access annotations that were collected during evaluation, indexed by
  /// instance location and evaluation path
  auto annotations() const -> const auto & { return this->annotations_; }

  struct Location {
    auto operator<(const Location &other) const noexcept -> bool {
      // Perform a lexicographical comparison
      return std::tie(instance_location, evaluate_path) <
             std::tie(other.instance_location, other.evaluate_path);
    }

    const sourcemeta::core::WeakPointer instance_location;
    const sourcemeta::core::WeakPointer evaluate_path;
  };

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
  std::set<sourcemeta::core::WeakPointer> mask;
  std::map<Location, std::vector<sourcemeta::core::JSON>> annotations_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
}; // namespace sourcemeta::blaze

/// @ingroup compiler
///
/// An evaluation callback that reports a trace of execution. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
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
///     schema, sourcemeta::core::schema_official_walker,
///     sourcemeta::core::schema_official_resolver,
///     sourcemeta::core::default_schema_compiler)};
///
/// const sourcemeta::core::JSON instance{5};
///
/// sourcemeta::blaze::TraceOutput output;
/// sourcemeta::blaze::Evaluator evaluator;
/// const auto result{evaluator.validate(
///   schema_template, instance, std::ref(output))};
///
/// if (!result) {
///   for (const auto &entry : output) {
//      switch (entry.type) {
//        case sourcemeta::blaze::TraceOutput::EntryType::Push:
//          std::cerr << "-> (push) ";
//          break;
//        case sourcemeta::blaze::TraceOutput::EntryType::Pass:
//          std::cerr << "<- (pass) ";
//          break;
//        case sourcemeta::blaze::TraceOutput::EntryType::Fail:
//          std::cerr << "<- (fail) ";
//          break;
//      }
///
///     std::cerr << entry.name << "\n";
///     std::cerr << entry.keyword_location << "\n";
///     sourcemeta::core::stringify(entry.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::core::stringify(entry.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_BLAZE_COMPILER_EXPORT TraceOutput {
public:
  TraceOutput(const sourcemeta::core::WeakPointer &base =
                  sourcemeta::core::empty_weak_pointer);

  // Prevent accidental copies
  TraceOutput(const TraceOutput &) = delete;
  auto operator=(const TraceOutput &) -> TraceOutput & = delete;

  enum class EntryType { Push, Pass, Fail, Annotation };

  struct Entry {
    const EntryType type;
    const std::string_view name;
    const sourcemeta::core::WeakPointer instance_location;
    const sourcemeta::core::WeakPointer evaluate_path;
    const std::string keyword_location;
    const std::optional<sourcemeta::core::JSON> annotation;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const sourcemeta::core::WeakPointer &evaluate_path,
                  const sourcemeta::core::WeakPointer &instance_location,
                  const sourcemeta::core::JSON &annotation) -> void;

  using container_type = typename std::vector<Entry>;
  using const_iterator = typename container_type::const_iterator;
  auto begin() const -> const_iterator;
  auto end() const -> const_iterator;
  auto cbegin() const -> const_iterator;
  auto cend() const -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const sourcemeta::core::WeakPointer base_;
  container_type output;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup compiler
///
/// This function translates a step execution into a human-readable string.
/// Useful as the building block for producing user-friendly evaluation results.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
describe(const bool valid, const Instruction &step,
         const sourcemeta::core::WeakPointer &evaluate_path,
         const sourcemeta::core::WeakPointer &instance_location,
         const sourcemeta::core::JSON &instance,
         const sourcemeta::core::JSON &annotation) -> std::string;

} // namespace sourcemeta::blaze

#endif
