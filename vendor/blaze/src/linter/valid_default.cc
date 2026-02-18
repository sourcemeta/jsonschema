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
  using Known = sourcemeta::core::Vocabularies::Known;
  // Technically, the `default` keyword goes back to Draft 1, but Blaze
  // only supports Draft 4 and later
  if (!vocabularies.contains(Known::JSON_Schema_2020_12_Meta_Data) &&
      !vocabularies.contains(Known::JSON_Schema_2019_09_Meta_Data) &&
      !vocabularies.contains(Known::JSON_Schema_Draft_7) &&
      !vocabularies.contains(Known::JSON_Schema_Draft_6) &&
      !vocabularies.contains(Known::JSON_Schema_Draft_4)) {
    return false;
  }

  if (!schema.is_object() || !schema.defines("default")) {
    return false;
  }

  // We have to ignore siblings to `$ref`
  if (vocabularies.contains(Known::JSON_Schema_Draft_7) ||
      vocabularies.contains(Known::JSON_Schema_Draft_6) ||
      vocabularies.contains(Known::JSON_Schema_Draft_4)) {
    if (schema.defines("$ref")) {
      return false;
    }
  }

  const auto &instance{schema.at("default")};

  if (frame.standalone()) {
    const auto base{frame.uri(location.pointer)};
    assert(base.has_value());
    const auto schema_template{compile(root, walker, resolver, this->compiler_,
                                       frame, base.value().get(),
                                       Mode::Exhaustive)};
    SimpleOutput output{instance};
    Evaluator evaluator;
    const auto result{
        evaluator.validate(schema_template, instance, std::ref(output))};
    if (result) {
      return false;
    }

    std::ostringstream message;
    for (const auto &entry : output) {
      message << entry.message << "\n";
      message << "  at instance location \"";
      sourcemeta::core::stringify(entry.instance_location, message);
      message << "\"\n";
      message << "  at evaluate path \"";
      sourcemeta::core::stringify(entry.evaluate_path, message);
      message << "\"\n";
    }

    return {{{"default"}}, std::move(message).str()};
  }

  const auto &root_base_dialect{
      frame.traverse(frame.root()).value_or(location).get().base_dialect};
  std::string_view default_id{location.base};
  if (!sourcemeta::core::identify(root, root_base_dialect).empty() ||
      default_id.empty()) {
    default_id = "";
  }

  sourcemeta::core::WeakPointer base;
  const auto subschema{
      sourcemeta::core::wrap(root, frame, location, resolver, base)};
  const auto schema_template{compile(subschema, walker, resolver,
                                     this->compiler_, Mode::Exhaustive,
                                     location.dialect, default_id)};
  SimpleOutput output{instance, base};
  Evaluator evaluator;
  const auto result{
      evaluator.validate(schema_template, instance, std::ref(output))};
  if (result) {
    return false;
  }

  std::ostringstream message;
  for (const auto &entry : output) {
    message << entry.message << "\n";
    message << "  at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, message);
    message << "\"\n";
    message << "  at evaluate path \"";
    sourcemeta::core::stringify(entry.evaluate_path, message);
    message << "\"\n";
  }

  return {{{"default"}}, std::move(message).str()};
}

auto ValidDefault::transform(
    sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaTransformRule::Result &) const -> void {
  schema.erase("default");
}

} // namespace sourcemeta::blaze
