#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>

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
    const sourcemeta::core::SchemaFrame &,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) const -> bool {
  if (!vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/meta-data") &&
      !vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/meta-data") &&
      !vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
      !vocabularies.contains("http://json-schema.org/draft-06/schema#")) {
    return false;
  }

  if (!schema.defines("examples") || !schema.at("examples").is_array() ||
      schema.at("examples").empty()) {
    return false;
  }

  const auto subschema{sourcemeta::core::wrap(root, location.pointer, resolver,
                                              location.dialect)};
  const auto schema_template{compile(subschema, walker, resolver,
                                     this->compiler_, Mode::FastValidation,
                                     location.dialect)};

  Evaluator evaluator;
  for (const auto &example : schema.at("examples").as_array()) {
    if (!evaluator.validate(schema_template, example)) {
      return true;
    }
  }

  return false;
}

auto ValidExamples::transform(sourcemeta::core::JSON &schema) const -> void {
  schema.erase("examples");
}

} // namespace sourcemeta::blaze
