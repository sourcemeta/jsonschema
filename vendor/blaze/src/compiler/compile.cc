#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator_context.h>

#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <algorithm> // std::move, std::sort, std::unique
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
      return {make<AssertionFail>(context, schema_context, dynamic_context,
                                  ValueNone{})};
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

auto precompile(
    const sourcemeta::blaze::Context &context,
    sourcemeta::blaze::SchemaContext &schema_context,
    const sourcemeta::blaze::DynamicContext &dynamic_context,
    const sourcemeta::jsontoolkit::ReferenceFrame::value_type &entry)
    -> sourcemeta::blaze::Template {
  const sourcemeta::jsontoolkit::URI anchor_uri{entry.first.second};
  const auto label{sourcemeta::blaze::EvaluationContext{}.hash(
      schema_resource_id(context,
                         anchor_uri.recompose_without_fragment().value_or("")),
      std::string{anchor_uri.fragment().value_or("")})};
  schema_context.labels.insert(label);

  // Configure a schema context that corresponds to the
  // schema resource that we are precompiling
  auto subschema{
      sourcemeta::jsontoolkit::get(context.root, entry.second.pointer)};
  auto nested_vocabularies{sourcemeta::jsontoolkit::vocabularies(
      subschema, context.resolver, entry.second.dialect)};
  const sourcemeta::blaze::SchemaContext nested_schema_context{
      entry.second.relative_pointer,
      std::move(subschema),
      std::move(nested_vocabularies),
      entry.second.base,
      {},
      {}};

  return {make<sourcemeta::blaze::ControlMark>(
      context, nested_schema_context, dynamic_context,
      sourcemeta::blaze::ValueUnsignedInteger{label},
      sourcemeta::blaze::compile(context, nested_schema_context,
                                 sourcemeta::blaze::relative_dynamic_context,
                                 sourcemeta::jsontoolkit::empty_pointer,
                                 sourcemeta::jsontoolkit::empty_pointer,
                                 entry.first.second))};
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

  std::set<sourcemeta::jsontoolkit::Pointer> unevaluated_properties_schemas;
  std::set<sourcemeta::jsontoolkit::Pointer> unevaluated_items_schemas;

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

      if (subschema.defines("unevaluatedProperties") &&
          sourcemeta::jsontoolkit::is_schema(
              subschema.at("unevaluatedProperties"))) {

        // No need to consider `unevaluatedProperties` if it has a sibling
        // `additionalProperties`. By definition, nothing could remain
        // unevaluated.

        if (entry.vocabularies.contains(
                "https://json-schema.org/draft/2019-09/vocab/applicator") &&
            subschema.defines("additionalProperties")) {
          continue;
        }

        if (entry.vocabularies.contains(
                "https://json-schema.org/draft/2020-12/vocab/applicator") &&
            subschema.defines("additionalProperties")) {
          continue;
        }

        unevaluated_properties_schemas.insert(entry.pointer);
      }

      if (subschema.defines("unevaluatedItems") &&
          sourcemeta::jsontoolkit::is_schema(
              subschema.at("unevaluatedItems"))) {

        // No need to consider `unevaluatedItems` if it has a
        // sibling `items` schema (or its older equivalents).
        // By definition, nothing could remain unevaluated.

        if (entry.vocabularies.contains(
                "https://json-schema.org/draft/2020-12/vocab/applicator") &&
            subschema.defines("items")) {
          continue;
        } else if (entry.vocabularies.contains("https://json-schema.org/draft/"
                                               "2019-09/vocab/applicator")) {
          if (subschema.defines("items") &&
              sourcemeta::jsontoolkit::is_schema(subschema.at("items"))) {
            continue;
          } else if (subschema.defines("items") &&
                     subschema.at("items").is_array() &&
                     subschema.defines("additionalItems")) {
            continue;
          }
        }

        unevaluated_items_schemas.insert(entry.pointer);
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

  // Calculate the top static reference destinations for precompilation purposes
  std::map<std::string, std::size_t> static_references_count;
  for (const auto &reference : references) {
    if (reference.first.first !=
            sourcemeta::jsontoolkit::ReferenceType::Static ||
        !frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
                         reference.second.destination})) {
      continue;
    }

    const auto &entry{frame.at({sourcemeta::jsontoolkit::ReferenceType::Static,
                                reference.second.destination})};
    for (const auto &subreference : references) {
      if (subreference.first.second.starts_with(entry.pointer)) {
        static_references_count[reference.second.destination] += 1;
      }
    }
  }
  std::vector<std::pair<std::string, std::size_t>> top_static_destinations(
      static_references_count.cbegin(), static_references_count.cend());
  std::sort(top_static_destinations.begin(), top_static_destinations.end(),
            [](const auto &left, const auto &right) {
              return left.second > right.second;
            });
  constexpr auto MAXIMUM_NUMBER_OF_SCHEMAS_TO_PRECOMPILE{5};
  std::set<std::string> precompiled_static_schemas;
  for (auto iterator = top_static_destinations.cbegin();
       iterator != top_static_destinations.cend() &&
       iterator != top_static_destinations.cbegin() +
                       MAXIMUM_NUMBER_OF_SCHEMAS_TO_PRECOMPILE;
       ++iterator) {
    // Only consider highly referenced schemas
    if (iterator->second > 100) {
      precompiled_static_schemas.insert(iterator->first);
    }
  }

  const Context context{result,
                        frame,
                        references,
                        std::move(resources),
                        walker,
                        resolver,
                        compiler,
                        mode,
                        uses_dynamic_scopes,
                        unevaluated_properties_schemas,
                        unevaluated_items_schemas,
                        std::move(precompiled_static_schemas)};
  const DynamicContext dynamic_context{relative_dynamic_context};
  Template compiler_template;

  for (const auto &destination : context.precompiled_static_schemas) {
    assert(frame.contains(
        {sourcemeta::jsontoolkit::ReferenceType::Static, destination}));
    const auto match{frame.find(
        {sourcemeta::jsontoolkit::ReferenceType::Static, destination})};
    for (auto &&substep :
         precompile(context, schema_context, dynamic_context, *match)) {
      compiler_template.push_back(std::move(substep));
    }
  }

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

      for (auto &&substep :
           precompile(context, schema_context, dynamic_context, entry)) {
        compiler_template.push_back(std::move(substep));
      }
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

} // namespace sourcemeta::blaze
