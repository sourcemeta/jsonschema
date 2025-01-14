#ifndef SOURCEMETA_BLAZE_EVALUATOR_H_
#define SOURCEMETA_BLAZE_EVALUATOR_H_

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/blaze/evaluator_error.h>
#include <sourcemeta/blaze/evaluator_instruction.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <cstdint>    // std::uint8_t
#include <functional> // std::function, std::reference_wrapper
#include <map>        // std::map
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
  Instructions instructions;
  bool dynamic;
  bool track;
};

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
/// - The evaluation path
/// - The instance location
/// - The annotation result, if any (otherwise null)
///
/// You can use this callback mechanism to implement arbitrary output formats.
using Callback =
    std::function<void(const EvaluationType, bool, const Instruction &,
                       const sourcemeta::jsontoolkit::WeakPointer &,
                       const sourcemeta::jsontoolkit::WeakPointer &,
                       const sourcemeta::jsontoolkit::JSON &)>;

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
  /// #include <sourcemeta/jsontoolkit/json.h>
  /// #include <sourcemeta/jsontoolkit/jsonschema.h>
  ///
  /// #include <cassert>
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
  /// sourcemeta::blaze::Evaluator evaluator;
  /// const sourcemeta::jsontoolkit::JSON instance{"foo bar"};
  /// const auto result{evaluator.validate(schema_template, instance)};
  /// assert(result);
  /// ```
  auto validate(const Template &schema,
                const sourcemeta::jsontoolkit::JSON &instance) -> bool;

  /// This method evaluates a schema compiler template, executing the given
  /// callback at every step of the way. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/evaluator.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/jsontoolkit/json.h>
  /// #include <sourcemeta/jsontoolkit/jsonschema.h>
  ///
  /// #include <cassert>
  /// #include <iostream>
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
  /// static auto callback(
  ///     bool result,
  ///     const sourcemeta::blaze::Instruction &instruction,
  ///     const sourcemeta::jsontoolkit::Pointer &evaluate_path,
  ///     const sourcemeta::jsontoolkit::Pointer &instance_location,
  ///     const sourcemeta::jsontoolkit::JSON &document,
  ///     const sourcemeta::jsontoolkit::JSON &annotation) -> void {
  ///   std::cout << "TYPE: " << (result ? "Success" : "Failure") << "\n";
  ///   std::cout << "INSTRUCTION:\n";
  ///   sourcemeta::jsontoolkit::prettify(sourcemeta::blaze::to_json({instruction}),
  ///                                     std::cout);
  ///   std::cout << "\nEVALUATE PATH:";
  ///   sourcemeta::jsontoolkit::stringify(evaluate_path, std::cout);
  ///   std::cout << "\nINSTANCE LOCATION:";
  ///   sourcemeta::jsontoolkit::stringify(instance_location, std::cout);
  ///   std::cout << "\nANNOTATION:\n";
  ///   sourcemeta::jsontoolkit::prettify(annotation, std::cout);
  ///   std::cout << "\n";
  /// }
  ///
  /// sourcemeta::blaze::Evaluator evaluator;
  /// const sourcemeta::jsontoolkit::JSON instance{"foo bar"};
  /// const auto result{evaluator.validate(
  ///   schema_template, instance, callback)};
  ///
  /// assert(result);
  /// ```
  auto validate(const Template &schema,
                const sourcemeta::jsontoolkit::JSON &instance,
                const Callback &callback) -> bool;

  // All of these members are considered internal and no
  // client must depend on them
#ifndef DOXYGEN
  static const sourcemeta::jsontoolkit::JSON null;
  static const sourcemeta::jsontoolkit::JSON empty_string;

  auto
  hash(const std::size_t resource,
       const sourcemeta::jsontoolkit::JSON::String &fragment) const noexcept
      -> std::size_t;

  auto evaluate(const sourcemeta::jsontoolkit::JSON *target) -> void;
  auto is_evaluated(const sourcemeta::jsontoolkit::JSON *target) const -> bool;
  auto unevaluate() -> void;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  sourcemeta::jsontoolkit::WeakPointer evaluate_path;
  sourcemeta::jsontoolkit::WeakPointer instance_location;
  const std::hash<sourcemeta::jsontoolkit::JSON::String> hasher_{};
  std::vector<std::size_t> resources;
  std::map<std::size_t, const std::reference_wrapper<const Instructions>>
      labels;

  struct Evaluation {
    const sourcemeta::jsontoolkit::JSON *instance;
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
