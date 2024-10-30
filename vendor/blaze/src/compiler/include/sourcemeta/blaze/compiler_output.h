#ifndef SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_
#define SOURCEMETA_BLAZE_COMPILER_OUTPUT_H_

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <sourcemeta/blaze/evaluator.h>

#include <set>    // std::set
#include <string> // std::string
#include <vector> // std::vector

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
/// sourcemeta::blaze::ErrorTraceOutput output;
/// const auto result{sourcemeta::blaze::evaluate(
///   schema_template, instance, std::ref(output))};
///
/// if (!result) {
///   for (const auto &trace : output) {
///     std::cerr << trace.message << "\n";
///     sourcemeta::jsontoolkit::stringify(trace.instance_location, std::cerr);
///     std::cerr << "\n";
///     sourcemeta::jsontoolkit::stringify(trace.evaluate_path, std::cerr);
///     std::cerr << "\n";
///   }
/// }
/// ```
class SOURCEMETA_BLAZE_COMPILER_EXPORT ErrorTraceOutput {
public:
  ErrorTraceOutput(const sourcemeta::jsontoolkit::JSON &instance,
                   const sourcemeta::jsontoolkit::WeakPointer &base =
                       sourcemeta::jsontoolkit::empty_weak_pointer);

  // Prevent accidental copies
  ErrorTraceOutput(const ErrorTraceOutput &) = delete;
  auto operator=(const ErrorTraceOutput &) -> ErrorTraceOutput & = delete;

  struct Entry {
    const std::string message;
    const sourcemeta::jsontoolkit::WeakPointer instance_location;
    const sourcemeta::jsontoolkit::WeakPointer evaluate_path;
  };

  auto operator()(const EvaluationType type, const bool result,
                  const Template::value_type &step,
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
/// This function translates a step execution into a human-readable string.
/// Useful as the building block for producing user-friendly evaluation results.
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
describe(const bool valid, const Template::value_type &step,
         const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
         const sourcemeta::jsontoolkit::WeakPointer &instance_location,
         const sourcemeta::jsontoolkit::JSON &instance,
         const sourcemeta::jsontoolkit::JSON &annotation) -> std::string;

} // namespace sourcemeta::blaze

#endif
