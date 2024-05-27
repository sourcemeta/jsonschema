#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert> // assert
#include <utility> // std::move

#include "compile_helpers.h"

namespace {

auto compile_subschema(
    const sourcemeta::jsontoolkit::SchemaCompilerContext &context)
    -> sourcemeta::jsontoolkit::SchemaCompilerTemplate {
  using namespace sourcemeta::jsontoolkit;
  assert(is_schema(context.schema));

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (context.schema.is_boolean()) {
    if (context.schema.to_boolean()) {
      return {};
    } else {
      return {make<SchemaCompilerAssertionFail>(
          context, SchemaCompilerValueNone{}, {},
          SchemaCompilerTargetType::Instance)};
    }
  }

  SchemaCompilerTemplate steps;
  for (const auto &entry :
       SchemaKeywordIterator{context.schema, context.walker, context.resolver,
                             context.default_dialect}) {
    assert(entry.pointer.back().is_property());
    const auto &keyword{entry.pointer.back().to_property()};
    for (auto &&step : context.compiler(
             {keyword, context.schema, entry.vocabularies, entry.value,
              context.root, context.base,
              context.relative_pointer.concat({keyword}),
              context.base_schema_location, context.base_instance_location,
              context.labels, context.frame, context.references, context.walker,
              context.resolver, context.compiler, context.default_dialect})) {
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
  const JSON result{bundle(schema, walker, resolver, default_dialect).get()};

  // Perform framing to resolve references later on
  ReferenceFrame frame;
  ReferenceMap references;
  sourcemeta::jsontoolkit::frame(result, frame, references, walker, resolver,
                                 default_dialect)
      .wait();

  const std::string base{
      URI{sourcemeta::jsontoolkit::id(schema, resolver, default_dialect)
              .get()
              .value_or("")}
          .canonicalize()
          .recompose()};

  assert(frame.contains({ReferenceType::Static, base}));
  const auto root_frame_entry{frame.at({ReferenceType::Static, base})};

  return compile_subschema(
      {"",
       result,
       vocabularies(schema, resolver, default_dialect).get(),
       JSON{nullptr},
       result,
       root_frame_entry.base,
       {},
       {},
       {},
       {},
       frame,
       references,
       walker,
       resolver,
       compiler,
       root_frame_entry.dialect});
}

auto compile(const SchemaCompilerContext &context, const Pointer &schema_suffix,
             const Pointer &instance_suffix,
             const std::optional<std::string> &uri) -> SchemaCompilerTemplate {
  // Determine URI of the destination after recursion
  const std::string destination{
      uri.has_value()
          ? URI{uri.value()}.canonicalize().recompose()
          : to_uri(context.relative_pointer.concat(schema_suffix), context.base)
                .canonicalize()
                .recompose()};

  // Otherwise the recursion attempt is non-sense
  assert(context.frame.contains({ReferenceType::Static, destination}));
  const auto &entry{context.frame.at({ReferenceType::Static, destination})};

  const auto &new_schema{get(context.root, entry.pointer)};
  return compile_subschema(
      {context.keyword, new_schema,
       vocabularies(new_schema, context.resolver, entry.dialect).get(),
       context.value, context.root, entry.base, entry.relative_pointer,
       context.keyword.empty()
           ? context.base_schema_location.concat(schema_suffix)
           : context.base_schema_location.concat({context.keyword})
                 .concat(schema_suffix),
       context.base_instance_location.concat(instance_suffix), context.labels,
       context.frame, context.references, context.walker, context.resolver,
       context.compiler, entry.dialect});
}

} // namespace sourcemeta::jsontoolkit
