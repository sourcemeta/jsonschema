#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator_context.h>

#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <algorithm> // std::move, std::any_of, std::sort, std::unique
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <utility>   // std::move

#include "compile_helpers.h"

namespace {

auto compile_subschema(const sourcemeta::blaze::Context &context,
                       const sourcemeta::blaze::SchemaContext &schema_context,
                       const sourcemeta::blaze::DynamicContext &dynamic_context,
                       const std::optional<std::string> &default_dialect)
    -> sourcemeta::blaze::Template {
  using namespace sourcemeta::blaze;
  assert(is_schema(schema_context.schema));

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (schema_context.schema.is_boolean()) {
    if (schema_context.schema.to_boolean()) {
      return {};
    } else {
      return {make<AssertionFail>(true, context, schema_context,
                                  dynamic_context, ValueNone{})};
    }
  }

  Template steps;
  for (const auto &entry : sourcemeta::jsontoolkit::SchemaKeywordIterator{
           schema_context.schema, context.walker, context.resolver,
           default_dialect}) {
    assert(entry.pointer.back().is_property());
    const auto &keyword{entry.pointer.back().to_property()};
    // Bases must not contain fragments
    assert(!schema_context.base.fragment().has_value());
    for (auto &&step : context.compiler(
             context,
             {schema_context.relative_pointer.concat({keyword}),
              schema_context.schema, entry.vocabularies, schema_context.base,
              // TODO: This represents a copy
              schema_context.labels, schema_context.references},
             {keyword, dynamic_context.base_schema_location,
              dynamic_context.base_instance_location})) {
      // Just a sanity check to ensure every keyword location is indeed valid
      assert(context.frame.contains(
          {sourcemeta::jsontoolkit::ReferenceType::Static,
           std::visit([](const auto &value) { return value.keyword_location; },
                      step)}));
      steps.push_back(std::move(step));
    }
  }

  return steps;
}

} // namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::SchemaWalker &walker,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const Compiler &compiler, const Mode mode,
             const std::optional<std::string> &default_dialect) -> Template {
  assert(is_schema(schema));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here
  const sourcemeta::jsontoolkit::JSON result{sourcemeta::jsontoolkit::bundle(
      schema, walker, resolver, sourcemeta::jsontoolkit::BundleOptions::Default,
      default_dialect)};

  // Perform framing to resolve references later on
  sourcemeta::jsontoolkit::ReferenceFrame frame;
  sourcemeta::jsontoolkit::ReferenceMap references;
  sourcemeta::jsontoolkit::frame(result, frame, references, walker, resolver,
                                 default_dialect);

  const std::string base{sourcemeta::jsontoolkit::URI{
      sourcemeta::jsontoolkit::identify(
          schema, resolver,
          sourcemeta::jsontoolkit::IdentificationStrategy::Strict,
          default_dialect)
          .value_or("")}
                             .canonicalize()
                             .recompose()};

  assert(
      frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static, base}));
  const auto root_frame_entry{
      frame.at({sourcemeta::jsontoolkit::ReferenceType::Static, base})};

  // Check whether dynamic referencing takes places in this schema. If not,
  // we can avoid the overhead of keeping track of dynamics scopes, etc
  bool uses_dynamic_scopes{false};
  for (const auto &reference : references) {
    if (reference.first.first ==
        sourcemeta::jsontoolkit::ReferenceType::Dynamic) {
      uses_dynamic_scopes = true;
      break;
    }
  }

  SchemaContext schema_context{
      sourcemeta::jsontoolkit::empty_pointer,
      result,
      vocabularies(schema, resolver, root_frame_entry.dialect),
      sourcemeta::jsontoolkit::URI{root_frame_entry.base}
          .canonicalize()
          .recompose(),
      {},
      {}};

  bool uses_unevaluated_properties{false};
  bool uses_unevaluated_items{false};
  if (schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/core") ||
      schema_context.vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/core")) {
    for (const auto &entry : sourcemeta::jsontoolkit::SchemaIterator{
             result, walker, resolver, default_dialect}) {
      if (!entry.vocabularies.contains(
              "https://json-schema.org/draft/2019-09/vocab/applicator") &&
          !entry.vocabularies.contains(
              "https://json-schema.org/draft/2020-12/vocab/unevaluated")) {
        continue;
      }

      const auto &subschema{
          sourcemeta::jsontoolkit::get(result, entry.pointer)};
      if (!subschema.is_object()) {
        continue;
      }

      if (!uses_unevaluated_properties &&
          subschema.defines("unevaluatedProperties") &&
          sourcemeta::jsontoolkit::is_schema(
              subschema.at("unevaluatedProperties"))) {
        uses_unevaluated_properties = true;
      }

      if (!uses_unevaluated_items && subschema.defines("unevaluatedItems") &&
          sourcemeta::jsontoolkit::is_schema(
              subschema.at("unevaluatedItems"))) {
        uses_unevaluated_items = true;
      }
    }
  }

  std::vector<std::string> resources;
  for (const auto &entry : frame) {
    if (entry.second.type ==
        sourcemeta::jsontoolkit::ReferenceEntryType::Resource) {
      resources.push_back(entry.first.second);
    }
  }

  // Rule out any duplicates as we will use this list as the
  // source for a perfect hash function on schema resources.
  std::sort(resources.begin(), resources.end());
  resources.erase(std::unique(resources.begin(), resources.end()),
                  resources.end());
  assert(resources.size() ==
         std::set<std::string>(resources.cbegin(), resources.cend()).size());

  const Context context{result,
                        frame,
                        references,
                        std::move(resources),
                        walker,
                        resolver,
                        compiler,
                        mode,
                        uses_dynamic_scopes,
                        uses_unevaluated_properties,
                        uses_unevaluated_items};
  const DynamicContext dynamic_context{relative_dynamic_context};
  Template compiler_template;

  if (uses_dynamic_scopes &&
      (schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2019-09/vocab/core") ||
       schema_context.vocabularies.contains(
           "https://json-schema.org/draft/2020-12/vocab/core"))) {
    for (const auto &entry : frame) {
      // We are only trying to find dynamic anchors
      if (entry.second.type !=
              sourcemeta::jsontoolkit::ReferenceEntryType::Anchor ||
          entry.first.first !=
              sourcemeta::jsontoolkit::ReferenceType::Dynamic) {
        continue;
      }

      const sourcemeta::jsontoolkit::URI anchor_uri{entry.first.second};
      const auto label{EvaluationContext{}.hash(
          schema_resource_id(
              context, anchor_uri.recompose_without_fragment().value_or("")),
          std::string{anchor_uri.fragment().value_or("")})};
      schema_context.labels.insert(label);

      // Configure a schema context that corresponds to the
      // schema resource that we are precompiling
      auto subschema{get(result, entry.second.pointer)};
      auto nested_vocabularies{
          vocabularies(subschema, resolver, entry.second.dialect)};
      const SchemaContext nested_schema_context{entry.second.relative_pointer,
                                                std::move(subschema),
                                                std::move(nested_vocabularies),
                                                entry.second.base,
                                                {},
                                                {}};

      compiler_template.push_back(make<ControlMark>(
          true, context, nested_schema_context, dynamic_context,
          ValueUnsignedInteger{label},
          compile(context, nested_schema_context, relative_dynamic_context,
                  sourcemeta::jsontoolkit::empty_pointer,
                  sourcemeta::jsontoolkit::empty_pointer, entry.first.second)));
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

auto compile(const Context &context, const SchemaContext &schema_context,
             const DynamicContext &dynamic_context,
             const sourcemeta::jsontoolkit::Pointer &schema_suffix,
             const sourcemeta::jsontoolkit::Pointer &instance_suffix,
             const std::optional<std::string> &uri) -> Template {
  // Determine URI of the destination after recursion
  const std::string destination{
      uri.has_value()
          ? sourcemeta::jsontoolkit::URI{uri.value()}.canonicalize().recompose()
          : to_uri(schema_context.relative_pointer.concat(schema_suffix),
                   schema_context.base)
                .canonicalize()
                .recompose()};

  // Otherwise the recursion attempt is non-sense
  if (!context.frame.contains(
          {sourcemeta::jsontoolkit::ReferenceType::Static, destination})) {
    throw sourcemeta::jsontoolkit::SchemaReferenceError(
        destination, schema_context.relative_pointer,
        "The target of the reference does not exist in the schema");
  }

  const auto &entry{context.frame.at(
      {sourcemeta::jsontoolkit::ReferenceType::Static, destination})};
  const auto &new_schema{get(context.root, entry.pointer)};

  if (!is_schema(new_schema)) {
    throw sourcemeta::jsontoolkit::SchemaReferenceError(
        destination, schema_context.relative_pointer,
        "The target of the reference is not a valid schema");
  }

  const sourcemeta::jsontoolkit::Pointer destination_pointer{
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location.concat(schema_suffix)
          : dynamic_context.base_schema_location
                .concat({dynamic_context.keyword})
                .concat(schema_suffix)};

  return compile_subschema(
      context,
      {entry.relative_pointer, new_schema,
       vocabularies(new_schema, context.resolver, entry.dialect),
       sourcemeta::jsontoolkit::URI{entry.base}
           .recompose_without_fragment()
           .value_or(""),
       // TODO: This represents a copy
       schema_context.labels, schema_context.references},
      {dynamic_context.keyword, destination_pointer,
       dynamic_context.base_instance_location.concat(instance_suffix)},
      entry.dialect);
}

ErrorTraceOutput::ErrorTraceOutput(
    const sourcemeta::jsontoolkit::JSON &instance,
    const sourcemeta::jsontoolkit::WeakPointer &base)
    : instance_{instance}, base_{base} {}

auto ErrorTraceOutput::begin() const -> const_iterator {
  return this->output.begin();
}

auto ErrorTraceOutput::end() const -> const_iterator {
  return this->output.end();
}

auto ErrorTraceOutput::cbegin() const -> const_iterator {
  return this->output.cbegin();
}

auto ErrorTraceOutput::cend() const -> const_iterator {
  return this->output.cend();
}

auto ErrorTraceOutput::operator()(
    const EvaluationType type, const bool result,
    const Template::value_type &step,
    const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
    const sourcemeta::jsontoolkit::WeakPointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &annotation) -> void {
  if (evaluate_path.empty()) {
    return;
  }

  assert(evaluate_path.back().is_property());

  if (type == EvaluationType::Pre) {
    assert(result);
    const auto &keyword{evaluate_path.back().to_property()};
    // To ease the output
    if (keyword == "oneOf" || keyword == "not") {
      this->mask.insert(evaluate_path);
    }
  } else if (type == EvaluationType::Post &&
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
      {describe(result, step, evaluate_path, instance_location, this->instance_,
                annotation),
       instance_location, std::move(effective_evaluate_path)});
}

} // namespace sourcemeta::blaze
