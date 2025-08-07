#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/compiler_output.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>
#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/jsonschema_frame.h>
#include <sourcemeta/core/jsonschema_transform.h>
#include <sourcemeta/core/jsonschema_types.h>

#include <cstddef>    // std::size_t
#include <functional> // std::ref, std::cref
#include <sstream>    // std::ostringstream
#include <utility>    // std::move

namespace sourcemeta::blaze {

ValidExamples::ValidExamples(Compiler compiler)
    : sourcemeta::core::
          SchemaTransformRule{"blaze/valid_examples",
                              "Only include instances in the `examples` array "
                              "that validate against the schema"},
      compiler_{std::move(compiler)} {};

auto ValidExamples::condition(
    const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
    const sourcemeta::core::Vocabularies &vocabularies,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) const
    -> sourcemeta::core::SchemaTransformRule::Result {
  if (!vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/meta-data") &&
      !vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/meta-data") &&
      !vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
      !vocabularies.contains("http://json-schema.org/draft-06/schema#")) {
    return false;
  }

  if (!schema.is_object() || !schema.defines("examples") ||
      !schema.at("examples").is_array() || schema.at("examples").empty()) {
    return false;
  }

  // We have to ignore siblings to `$ref`
  if (vocabularies.contains("http://json-schema.org/draft-07/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-06/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-04/schema#")) {
    if (schema.defines("$ref")) {
      return false;
    }
  }

  const auto &root_base_dialect{frame.traverse(location.root.value_or(""))
                                    .value_or(location)
                                    .get()
                                    .base_dialect};
  std::optional<std::string> default_id{location.base};
  if (sourcemeta::core::identify(root, root_base_dialect).has_value()) {
    // We want to only set a default identifier if the root schema does not
    // have an explicit identifier. Otherwise, we can get into corner case
    // when wrapping the schema
    default_id = std::nullopt;
  }

  const auto subschema{sourcemeta::core::wrap(root, location.pointer, resolver,
                                              location.dialect)};
  const auto schema_template{compile(subschema, walker, resolver,
                                     this->compiler_, Mode::FastValidation,
                                     location.dialect, default_id)};

  Evaluator evaluator;
  std::size_t cursor{0};
  for (const auto &example : schema.at("examples").as_array()) {
    const std::string ref{"$ref"};
    SimpleOutput output{example, {std::cref(ref)}};
    const auto result{
        evaluator.validate(schema_template, example, std::ref(output))};
    if (!result) {
      std::ostringstream message;
      message << "Invalid example instance at index " << cursor << "\n";
      output.stacktrace(message, "  ");
      return message.str();
    }

    cursor += 1;
  }

  return false;
}

auto ValidExamples::transform(sourcemeta::core::JSON &schema) const -> void {
  schema.erase("examples");
}

} // namespace sourcemeta::blaze
