#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>
#include <sourcemeta/blaze/output.h>

#include <functional> // std::ref, std::cref
#include <sstream>    // std::ostringstream

namespace sourcemeta::blaze {

ValidDefault::ValidDefault(Compiler compiler)
    : sourcemeta::core::SchemaTransformRule{"blaze/valid_default",
                                            "Only set a `default` value that "
                                            "validates against the schema"},
      compiler_{std::move(compiler)} {};

auto ValidDefault::condition(
    const sourcemeta::core::JSON &schema, const sourcemeta::core::JSON &root,
    const sourcemeta::core::Vocabularies &vocabularies,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver) const
    -> sourcemeta::core::SchemaTransformRule::Result {
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

  if (!schema.is_object() || !schema.defines("default")) {
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
  if (sourcemeta::core::identify(root, root_base_dialect).has_value() ||
      default_id.value().empty()) {
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

  const auto &instance{schema.at("default")};
  Evaluator evaluator;
  const std::string ref{"$ref"};
  SimpleOutput output{instance, {std::cref(ref)}};
  const auto result{
      evaluator.validate(schema_template, instance, std::ref(output))};
  if (result) {
    return false;
  }

  std::ostringstream message;
  output.stacktrace(message);
  return {{{"default"}}, std::move(message).str()};
}

auto ValidDefault::transform(
    sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaTransformRule::Result &) const -> void {
  schema.erase("default");
}

} // namespace sourcemeta::blaze
