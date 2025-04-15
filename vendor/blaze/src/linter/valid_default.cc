#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>

namespace sourcemeta::blaze {

ValidDefault::ValidDefault(Compiler compiler)
    : sourcemeta::core::SchemaTransformRule{"blaze/valid_default",
                                            "Only set a `default` value that "
                                            "validates against the schema"},
      compiler_{std::move(compiler)} {};

auto ValidDefault::condition(
    const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
    const sourcemeta::core::Vocabularies &vocabularies,
    const sourcemeta::core::SchemaFrame &,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) const -> bool {
  // Technically, the `default` keyword goes back to Draft 1, but Blaze
  // only supports Draft 4 and later
  if (!vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/meta-data") &&
      !vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/meta-data") &&
      !vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
      !vocabularies.contains("http://json-schema.org/draft-06/schema#") &&
      !vocabularies.contains("http://json-schema.org/draft-04/schema#")) {
    return false;
  }

  if (!schema.defines("default")) {
    return false;
  }

  const auto subschema{sourcemeta::core::wrap(root, location.pointer, resolver,
                                              location.dialect)};
  const auto schema_template{compile(subschema, walker, resolver,
                                     this->compiler_, Mode::FastValidation,
                                     location.dialect)};

  Evaluator evaluator;
  return !evaluator.validate(schema_template, schema.at("default"));
}

auto ValidDefault::transform(sourcemeta::core::JSON &schema) const -> void {
  schema.erase("default");
}

} // namespace sourcemeta::blaze
