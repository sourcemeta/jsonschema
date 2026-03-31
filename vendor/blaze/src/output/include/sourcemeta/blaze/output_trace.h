#ifndef SOURCEMETA_BLAZE_OUTPUT_TRACE_H_
#define SOURCEMETA_BLAZE_OUTPUT_TRACE_H_

#ifndef SOURCEMETA_BLAZE_OUTPUT_EXPORT
#include <sourcemeta/blaze/output_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/evaluator.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function, std::reference_wrapper
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::pair

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
/// const sourcemeta::core::JSON instance{5};
///
/// sourcemeta::blaze::TraceOutput output{
///     sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver,
///     [](const sourcemeta::blaze::TraceOutput::Entry &entry) {
///       std::cerr << entry.name << "\n";
///     }};
/// sourcemeta::blaze::Evaluator evaluator;
/// const auto result{evaluator.validate(
///   schema_template, instance, std::ref(output))};
/// ```
class SOURCEMETA_BLAZE_OUTPUT_EXPORT TraceOutput {
public:
  enum class EntryType : std::uint8_t { Push, Pass, Fail, Annotation };

  // NOLINTNEXTLINE(bugprone-exception-escape)
  struct Entry {
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const EntryType type;
    const std::string_view name;
    const sourcemeta::core::WeakPointer &instance_location;
    const sourcemeta::core::WeakPointer &evaluate_path;
    const std::string_view keyword_location;
    const sourcemeta::core::JSON &annotation;
    const std::pair<bool, std::optional<sourcemeta::core::Vocabularies::URI>>
        &vocabulary;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
  };

  // TODO(C++23): Use std::move_only_function when available in libc++
  using Callback = std::function<void(const Entry &)>;

  TraceOutput(
      sourcemeta::core::SchemaWalker walker,
      sourcemeta::core::SchemaResolver resolver, Callback callback,
      sourcemeta::core::WeakPointer base = sourcemeta::core::empty_weak_pointer,
      const std::optional<
          std::reference_wrapper<const sourcemeta::core::SchemaFrame>> &frame =
          std::nullopt);

  // Prevent accidental copies
  TraceOutput(const TraceOutput &) = delete;
  auto operator=(const TraceOutput &) -> TraceOutput & = delete;

  auto operator()(const EvaluationType type, const bool result,
                  const Instruction &step,
                  const InstructionExtra &step_metadata,
                  const sourcemeta::core::WeakPointer &evaluate_path,
                  const sourcemeta::core::WeakPointer &instance_location,
                  const sourcemeta::core::JSON &annotation) -> void;

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
  Callback callback_;
  std::vector<
      std::pair<bool, std::optional<sourcemeta::core::Vocabularies::URI>>>
      vocabulary_stack_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
