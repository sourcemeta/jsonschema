#ifndef SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_
#define SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>

#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view
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
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// #include <cassert>
/// #include <functional>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON instance{5};
///
/// sourcemeta::blaze::ErrorOutput output{instance};
/// sourcemeta::blaze::Evaluator evaluator;
/// const auto result{evaluator.validate(
///   schema_template, instance, std::ref(output))};
///
/// if (!result) {
///   for (const auto &entry : output) {
///     std::cerr << entry.message << "\n";
///     sourcemeta::jsontoolkit::stringify(entry.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::jsontoolkit::stringify(entry.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_BLAZE_COMPILER_EXPORT ErrorOutput {
public:
  ErrorOutput(const sourcemeta::jsontoolkit::JSON &instance,
              const sourcemeta::jsontoolkit::WeakPointer &base =
                  sourcemeta::jsontoolkit::empty_weak_pointer);

  // Prevent accidental copies
  ErrorOutput(const ErrorOutput &) = delete;
  auto operator=(const ErrorOutput &) -> ErrorOutput & = delete;

  struct Entry {
    const std::string message;
    const sourcemeta::jsontoolkit::WeakPointer instance_location;
    const sourcemeta::jsontoolkit::WeakPointer evaluate_path;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
                  const sourcemeta::jsontoolkit::WeakPointer &instance_location,
                  const sourcemeta::jsontoolkit::JSON &annotation) -> void;

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
  const sourcemeta::jsontoolkit::JSON &instance_;
  const sourcemeta::jsontoolkit::WeakPointer base_;
  container_type output;
  std::set<sourcemeta::jsontoolkit::WeakPointer> mask;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup compiler
///
/// An evaluation callback that reports a trace of execution. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
///
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// #include <cassert>
/// #include <functional>
///
/// const sourcemeta::jsontoolkit::JSON schema =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto schema_template{sourcemeta::blaze::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver,
///     sourcemeta::jsontoolkit::default_schema_compiler)};
///
/// const sourcemeta::jsontoolkit::JSON instance{5};
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
///     sourcemeta::jsontoolkit::stringify(entry.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::jsontoolkit::stringify(entry.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_BLAZE_COMPILER_EXPORT TraceOutput {
public:
  TraceOutput(const sourcemeta::jsontoolkit::WeakPointer &base =
                  sourcemeta::jsontoolkit::empty_weak_pointer);

  // Prevent accidental copies
  TraceOutput(const ErrorOutput &) = delete;
  auto operator=(const TraceOutput &) -> TraceOutput & = delete;

  enum class EntryType { Push, Pass, Fail };

  struct Entry {
    const EntryType type;
    const std::string_view name;
    const sourcemeta::jsontoolkit::WeakPointer instance_location;
    const sourcemeta::jsontoolkit::WeakPointer evaluate_path;
    const std::string keyword_location;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
                  const sourcemeta::jsontoolkit::WeakPointer &instance_location,
                  const sourcemeta::jsontoolkit::JSON &annotation) -> void;

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
  const sourcemeta::jsontoolkit::WeakPointer base_;
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
         const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
         const sourcemeta::jsontoolkit::WeakPointer &instance_location,
         const sourcemeta::jsontoolkit::JSON &instance,
         const sourcemeta::jsontoolkit::JSON &annotation) -> std::string;

} // namespace sourcemeta::blaze

#endif
