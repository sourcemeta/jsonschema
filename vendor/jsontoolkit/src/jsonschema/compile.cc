#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <utility> // std::move

#include "compile_helpers.h"

namespace {

auto compile_subschema(
    const sourcemeta::jsontoolkit::SchemaCompilerContext &context,
    const sourcemeta::jsontoolkit::SchemaCompilerSchemaContext &schema_context,
    const sourcemeta::jsontoolkit::SchemaCompilerDynamicContext
        &dynamic_context,
    const std::optional<std::string> &default_dialect)
    -> sourcemeta::jsontoolkit::SchemaCompilerTemplate {
  using namespace sourcemeta::jsontoolkit;
  assert(is_schema(schema_context.schema));

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (schema_context.schema.is_boolean()) {
    if (schema_context.schema.to_boolean()) {
      return {};
    } else {
      return {make<SchemaCompilerAssertionFail>(
          schema_context, dynamic_context, SchemaCompilerValueNone{}, {},
          SchemaCompilerTargetType::Instance)};
    }
  }

  SchemaCompilerTemplate steps;
  for (const auto &entry :
       SchemaKeywordIterator{schema_context.schema, context.walker,
                             context.resolver, default_dialect}) {
    assert(entry.pointer.back().is_property());
    const auto &keyword{entry.pointer.back().to_property()};
    for (auto &&step : context.compiler(
             context,
             {schema_context.relative_pointer.concat({keyword}),
              schema_context.schema, entry.vocabularies, schema_context.base,
              // TODO: This represents a copy
              schema_context.labels},
             {keyword, dynamic_context.base_schema_location,
              dynamic_context.base_instance_location})) {
      // Just a sanity check to ensure every keyword location is indeed valid
      assert(context.frame.contains(
          {ReferenceType::Static,
           std::visit([](const auto &value) { return value.keyword_location; },
                      step)}));
      steps.push_back(std::move(step));
    }
  }

  return steps;
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto compile(const JSON &schema, const SchemaWalker &walker,
             const SchemaResolver &resolver, const SchemaCompiler &compiler,
             const std::optional<std::string> &default_dialect)
    -> SchemaCompilerTemplate {
  assert(is_schema(schema));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here
  const JSON result{
      bundle(schema, walker, resolver, BundleOptions::Default, default_dialect)
          .get()};

  // Perform framing to resolve references later on
  ReferenceFrame frame;
  ReferenceMap references;
  sourcemeta::jsontoolkit::frame(result, frame, references, walker, resolver,
                                 default_dialect)
      .wait();

  const std::string base{
      URI{sourcemeta::jsontoolkit::id(
              schema, resolver,
              sourcemeta::jsontoolkit::IdentificationStrategy::Strict,
              default_dialect)
              .get()
              .value_or("")}
          .canonicalize()
          .recompose()};

  assert(frame.contains({ReferenceType::Static, base}));
  const auto root_frame_entry{frame.at({ReferenceType::Static, base})};

  return compile_subschema(
      {result, frame, references, walker, resolver, compiler},
      {empty_pointer,
       result,
       vocabularies(schema, resolver, root_frame_entry.dialect).get(),
       root_frame_entry.base,
       {}},
      relative_dynamic_context, root_frame_entry.dialect);
}

auto compile(const SchemaCompilerContext &context,
             const SchemaCompilerSchemaContext &schema_context,
             const SchemaCompilerDynamicContext &dynamic_context,
             const Pointer &schema_suffix, const Pointer &instance_suffix,
             const std::optional<std::string> &uri) -> SchemaCompilerTemplate {
  // Determine URI of the destination after recursion
  const std::string destination{
      uri.has_value()
          ? URI{uri.value()}.canonicalize().recompose()
          : to_uri(schema_context.relative_pointer.concat(schema_suffix),
                   schema_context.base)
                .canonicalize()
                .recompose()};

  const Pointer destination_pointer{
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location.concat(schema_suffix)
          : dynamic_context.base_schema_location
                .concat({dynamic_context.keyword})
                .concat(schema_suffix)};

  // Otherwise the recursion attempt is non-sense
  if (!context.frame.contains({ReferenceType::Static, destination})) {
    throw SchemaReferenceError(
        destination, destination_pointer,
        "The target of the reference does not exist in the schema");
  }

  const auto &entry{context.frame.at({ReferenceType::Static, destination})};

  const auto &new_schema{get(context.root, entry.pointer)};
  return compile_subschema(
      context,
      {entry.relative_pointer, new_schema,
       vocabularies(new_schema, context.resolver, entry.dialect).get(),
       entry.base,
       // TODO: This represents a copy
       schema_context.labels},
      {dynamic_context.keyword, destination_pointer,
       dynamic_context.base_instance_location.concat(instance_suffix)},
      entry.dialect);
}

} // namespace sourcemeta::jsontoolkit
