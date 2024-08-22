#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <algorithm> // std::move, std::any_of
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <utility>   // std::move

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
          context, schema_context, dynamic_context, SchemaCompilerValueNone{},
          {}, SchemaCompilerTargetType::Instance)};
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
      URI{sourcemeta::jsontoolkit::identify(
              schema, resolver,
              sourcemeta::jsontoolkit::IdentificationStrategy::Strict,
              default_dialect)
              .get()
              .value_or("")}
          .canonicalize()
          .recompose()};

  assert(frame.contains({ReferenceType::Static, base}));
  const auto root_frame_entry{frame.at({ReferenceType::Static, base})};

  // Check whether dynamic referencing takes places in this schema. If not,
  // we can avoid the overhead of keeping track of dynamics scopes, etc
  bool uses_dynamic_scopes{false};
  for (const auto &reference : references) {
    if (reference.first.first == ReferenceType::Dynamic) {
      uses_dynamic_scopes = true;
      break;
    }
  }

  const sourcemeta::jsontoolkit::SchemaCompilerContext context{
      result,   frame,    references,         walker,
      resolver, compiler, uses_dynamic_scopes};
  sourcemeta::jsontoolkit::SchemaCompilerSchemaContext schema_context{
      empty_pointer,
      result,
      vocabularies(schema, resolver, root_frame_entry.dialect).get(),
      root_frame_entry.base,
      {}};
  const sourcemeta::jsontoolkit::SchemaCompilerDynamicContext dynamic_context{
      relative_dynamic_context};
  sourcemeta::jsontoolkit::SchemaCompilerTemplate compiler_template;

  if (uses_dynamic_scopes &&
      (schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2019-09/vocab/core") ||
       schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2020-12/vocab/core"))) {
    for (const auto &entry : frame) {
      // We are only trying to find dynamic anchors
      if (entry.second.type != ReferenceEntryType::Anchor ||
          entry.first.first != ReferenceType::Dynamic) {
        continue;
      }

      const URI anchor_uri{entry.first.second};
      std::ostringstream name;
      name << anchor_uri.recompose_without_fragment().value_or("");
      name << '#';
      name << anchor_uri.fragment().value_or("");
      const auto label{std::hash<std::string>{}(name.str())};
      schema_context.labels.insert(label);

      // Configure a schema context that corresponds to the
      // schema resource that we are precompiling
      auto subschema{get(result, entry.second.pointer)};
      auto nested_vocabularies{
          vocabularies(subschema, resolver, entry.second.dialect).get()};
      const sourcemeta::jsontoolkit::SchemaCompilerSchemaContext
          nested_schema_context{entry.second.relative_pointer,
                                std::move(subschema),
                                std::move(nested_vocabularies),
                                entry.second.base,
                                {}};

      compiler_template.push_back(make<SchemaCompilerControlMark>(
          context, nested_schema_context, dynamic_context,
          SchemaCompilerValueUnsignedInteger{label},
          compile(context, nested_schema_context, relative_dynamic_context,
                  empty_pointer, empty_pointer, entry.first.second)));
    }
  }

  auto children{compile_subschema(context, schema_context, dynamic_context,
                                  root_frame_entry.dialect)};
  if (compiler_template.empty()) {
    return children;
  } else {
    compiler_template.reserve(compiler_template.size() + children.size());
    std::move(children.begin(), children.end(),
              std::back_inserter(compiler_template));
    return compiler_template;
  }
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

SchemaCompilerErrorTraceOutput::SchemaCompilerErrorTraceOutput(
    const JSON &instance, const Pointer &base)
    : instance_{instance}, base_{base} {}

auto SchemaCompilerErrorTraceOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto SchemaCompilerErrorTraceOutput::end() const -> const_iterator {
  return this->output.end();
}

auto SchemaCompilerErrorTraceOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto SchemaCompilerErrorTraceOutput::cend() const -> const_iterator {
  return this->output.cend();
}

auto SchemaCompilerErrorTraceOutput::operator()(
    const SchemaCompilerEvaluationType type, const bool result,
    const SchemaCompilerTemplate::value_type &step,
    const Pointer &evaluate_path, const Pointer &instance_location,
    const JSON &annotation) -> void {
  assert(!evaluate_path.empty());
  assert(evaluate_path.back().is_property());

  if (type == sourcemeta::jsontoolkit::SchemaCompilerEvaluationType::Pre) {
    assert(result);
    const auto &keyword{evaluate_path.back().to_property()};
    // To ease the output
    if (keyword == "oneOf" || keyword == "not") {
      this->mask.insert(evaluate_path);
    }
  } else if (type ==
                 sourcemeta::jsontoolkit::SchemaCompilerEvaluationType::Post &&
             this->mask.contains(evaluate_path)) {
    this->mask.erase(evaluate_path);
  }

  // Ignore successful or masked steps
  if (result || std::any_of(this->mask.cbegin(), this->mask.cend(),
                            [&evaluate_path](const auto &entry) {
                              return evaluate_path.starts_with(entry);
                            })) {
    return;
  }

  auto effective_evaluate_path{evaluate_path.resolve_from(this->base_)};
  if (effective_evaluate_path.empty()) {
    return;
  }

  this->output.push_back(
      {sourcemeta::jsontoolkit::describe(result, step, evaluate_path,
                                         instance_location, this->instance_,
                                         annotation),
       instance_location, std::move(effective_evaluate_path)});
}

} // namespace sourcemeta::jsontoolkit
