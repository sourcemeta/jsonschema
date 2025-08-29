#ifndef SOURCEMETA_BLAZE_OUTPUT_TRACE_H_
#define SOURCEMETA_BLAZE_OUTPUT_TRACE_H_

#ifndef SOURCEMETA_BLAZE_OUTPUT_EXPORT
#include <sourcemeta/blaze/output_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/evaluator.h>

#include <functional>  // std::reference_wrapper
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

namespace sourcemeta::blaze {

/// @ingroup output
///
/// An evaluation callback that reports a trace of execution. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/compiler.h>
/// #include <sourcemeta/blaze/evaluator.h>
/// #include <sourcemeta/blaze/output.h>
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
class SOURCEMETA_BLAZE_OUTPUT_EXPORT TraceOutput {
public:
  // Passing a frame of the schema is optional, but allows the output
  // formatter to give you back additional information
  TraceOutput(
      sourcemeta::core::SchemaWalker walker,
      sourcemeta::core::SchemaResolver resolver,
      sourcemeta::core::WeakPointer base = sourcemeta::core::empty_weak_pointer,
      const std::optional<
          std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame =
          std::nullopt);

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
    // Whether we were able to collect vocabulary information,
    // and the vocabulary URI, if any
    const std::pair<bool, std::optional<std::string>> vocabulary;
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
  const sourcemeta::core::SchemaWalker walker_;
  const sourcemeta::core::SchemaResolver resolver_;
  const sourcemeta::core::WeakPointer base_;
  const std::optional<
      std::reference_wrapper<const sourcemeta::core::SchemaFrame>>
      frame_;
  container_type output;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
