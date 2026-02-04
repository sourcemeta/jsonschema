#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/linter.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/jsonschema.h>

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
  using Known = sourcemeta::core::Vocabularies::Known;
  if (!vocabularies.contains(Known::JSON_Schema_2020_12_Meta_Data) &&
      !vocabularies.contains(Known::JSON_Schema_2019_09_Meta_Data) &&
      !vocabularies.contains(Known::JSON_Schema_Draft_7) &&
      !vocabularies.contains(Known::JSON_Schema_Draft_6)) {
    return false;
  }

  if (!schema.is_object() || !schema.defines("examples") ||
      !schema.at("examples").is_array() || schema.at("examples").empty()) {
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

  const auto &root_base_dialect{
      frame.traverse(frame.root()).value_or(location).get().base_dialect};
  std::string_view default_id{location.base};
  if (!sourcemeta::core::identify(root, root_base_dialect).empty() ||
      default_id.empty()) {
    // We want to only set a default identifier if the root schema does not
    // have an explicit identifier. Otherwise, we can get into corner case
    // when wrapping the schema
    default_id = "";
  }

  sourcemeta::core::WeakPointer base;
  const auto subschema{
      sourcemeta::core::wrap(root, frame, location, resolver, base)};
  // To avoid bundling twice in vain
  Tweaks tweaks{.assume_bundled = frame.standalone()};
  const auto schema_template{compile(subschema, walker, resolver,
                                     this->compiler_, Mode::Exhaustive,
                                     location.dialect, default_id, "", tweaks)};

  Evaluator evaluator;
  std::size_t cursor{0};
  for (const auto &example : schema.at("examples").as_array()) {
    SimpleOutput output{example, base};
    const auto result{
        evaluator.validate(schema_template, example, std::ref(output))};
    if (!result) {
      std::ostringstream message;
      message << "Invalid example instance at index " << cursor << "\n";
      for (const auto &entry : output) {
        message << "  " << entry.message << "\n";
        message << "  "
                << "  at instance location \"";
        sourcemeta::core::stringify(entry.instance_location, message);
        message << "\"\n";
        message << "  "
                << "  at evaluate path \"";
        sourcemeta::core::stringify(entry.evaluate_path, message);
        message << "\"\n";
      }

      return {{{"examples", cursor}}, std::move(message).str()};
    }

    cursor += 1;
  }

  return false;
}

auto ValidExamples::transform(
    sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaTransformRule::Result &) const -> void {
  schema.erase("examples");
}

} // namespace sourcemeta::blaze
