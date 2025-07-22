#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/jsonschema.h>

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
    -> sourcemeta::blaze::Instructions {
  using namespace sourcemeta::blaze;
  assert(is_schema(schema_context.schema));

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (schema_context.schema.is_boolean()) {
    if (schema_context.schema.to_boolean()) {
      return {};
    } else {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                   schema_context, dynamic_context, ValueNone{})};
    }
  }

  Instructions steps;
  for (const auto &entry : sourcemeta::core::SchemaKeywordIterator{
           schema_context.schema, context.walker, context.resolver,
           default_dialect}) {
    assert(entry.pointer.back().is_property());
    const auto &keyword{entry.pointer.back().to_property()};
    // Bases must not contain fragments
    assert(!schema_context.base.fragment().has_value());
    for (auto &&step : context.compiler(
             context,
             {.relative_pointer =
                  schema_context.relative_pointer.concat({keyword}),
              .schema = schema_context.schema,
              .vocabularies = entry.vocabularies,
              .base = schema_context.base,
              // TODO: This represents a copy
              .labels = schema_context.labels,
              .references = schema_context.references,
              .is_property_name = schema_context.is_property_name},
             {.keyword = keyword,
              .base_schema_location = dynamic_context.base_schema_location,
              .base_instance_location = dynamic_context.base_instance_location,
              .property_as_target = dynamic_context.property_as_target},
             steps)) {
      // Just a sanity check to ensure every keyword location is indeed valid
      assert(context.frame.locations().contains(
          {sourcemeta::core::SchemaReferenceType::Static,
           step.keyword_location}));
      steps.push_back(std::move(step));
    }
  }

  return steps;
}

auto precompile(
    const sourcemeta::blaze::Context &context,
    sourcemeta::blaze::SchemaContext &schema_context,
    const sourcemeta::blaze::DynamicContext &dynamic_context,
    const sourcemeta::core::SchemaFrame::Locations::value_type &entry)
    -> sourcemeta::blaze::Instructions {
  const sourcemeta::core::URI anchor_uri{entry.first.second};
  const auto label{sourcemeta::blaze::Evaluator{}.hash(
      schema_resource_id(context,
                         anchor_uri.recompose_without_fragment().value_or("")),
      std::string{anchor_uri.fragment().value_or("")})};
  schema_context.labels.insert(label);

  // Configure a schema context that corresponds to the
  // schema resource that we are precompiling
  auto subschema{sourcemeta::core::get(context.root, entry.second.pointer)};
  auto nested_vocabularies{sourcemeta::core::vocabularies(
      subschema, context.resolver, entry.second.dialect)};
  const sourcemeta::blaze::SchemaContext nested_schema_context{
      .relative_pointer = entry.second.relative_pointer,
      .schema = std::move(subschema),
      .vocabularies = std::move(nested_vocabularies),
      .base = entry.second.base,
      .labels = {},
      .references = {},
      .is_property_name = schema_context.is_property_name};

  return {make(sourcemeta::blaze::InstructionIndex::ControlMark, context,
               nested_schema_context, dynamic_context,
               sourcemeta::blaze::ValueUnsignedInteger{label},
               sourcemeta::blaze::compile(
                   context, nested_schema_context,
                   sourcemeta::blaze::relative_dynamic_context(),
                   sourcemeta::core::empty_pointer,
                   sourcemeta::core::empty_pointer, entry.first.second))};
}

} // namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const sourcemeta::core::SchemaFrame &frame, const Mode mode,
             const std::optional<std::string> &default_dialect,
             const std::optional<std::string> &default_id) -> Template {
  assert(is_schema(schema));

  const std::string base{sourcemeta::core::URI::canonicalize(
      sourcemeta::core::identify(
          schema, resolver,
          sourcemeta::core::SchemaIdentificationStrategy::Strict,
          default_dialect, default_id)
          .value_or(""))};

  assert(frame.locations().contains(
      {sourcemeta::core::SchemaReferenceType::Static, base}));
  const auto root_frame_entry{frame.locations().at(
      {sourcemeta::core::SchemaReferenceType::Static, base})};

  // Check whether dynamic referencing takes places in this schema. If not,
  // we can avoid the overhead of keeping track of dynamics scopes, etc
  bool uses_dynamic_scopes{false};
  for (const auto &reference : frame.references()) {
    if (reference.first.first ==
        sourcemeta::core::SchemaReferenceType::Dynamic) {
      uses_dynamic_scopes = true;
      break;
    }
  }

  SchemaContext schema_context{
      .relative_pointer = sourcemeta::core::empty_pointer,
      .schema = schema,
      .vocabularies = vocabularies(schema, resolver, root_frame_entry.dialect),
      .base = sourcemeta::core::URI::canonicalize(root_frame_entry.base),
      .labels = {},
      .references = {},
      .is_property_name = false};

  std::vector<std::string> resources;
  for (const auto &entry : frame.locations()) {
    if (entry.second.type ==
        sourcemeta::core::SchemaFrame::LocationType::Resource) {
      resources.push_back(entry.first.second);
    }
  }

  // Rule out any duplicates as we will use this list as the
  // source for a perfect hash function on schema resources.
  std::ranges::sort(resources);
  auto [first, last] = std::ranges::unique(resources);
  resources.erase(first, last);
  assert(resources.size() ==
         std::set<std::string>(resources.cbegin(), resources.cend()).size());

  // Calculate the top static reference destinations for precompilation purposes
  // TODO: Replace this logic with `.frame()` `destination_of` information
  std::map<std::string, std::size_t> static_references_count;
  for (const auto &reference : frame.references()) {
    if (reference.first.first !=
            sourcemeta::core::SchemaReferenceType::Static ||
        !frame.locations().contains(
            {sourcemeta::core::SchemaReferenceType::Static,
             reference.second.destination})) {
      continue;
    }

    const auto &entry{
        frame.locations().at({sourcemeta::core::SchemaReferenceType::Static,
                              reference.second.destination})};
    for (const auto &subreference : frame.references()) {
      if (subreference.first.second.starts_with(entry.pointer)) {
        static_references_count[reference.second.destination] += 1;
      }
    }
  }
  std::vector<std::pair<std::string, std::size_t>> top_static_destinations(
      static_references_count.cbegin(), static_references_count.cend());
  std::ranges::sort(top_static_destinations,
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

  auto unevaluated{
      sourcemeta::blaze::unevaluated(schema, frame, walker, resolver)};

  const Context context{.root = schema,
                        .frame = frame,
                        .resources = std::move(resources),
                        .walker = walker,
                        .resolver = resolver,
                        .compiler = compiler,
                        .mode = mode,
                        .uses_dynamic_scopes = uses_dynamic_scopes,
                        .unevaluated = std::move(unevaluated),
                        .precompiled_static_schemas =
                            std::move(precompiled_static_schemas)};
  const DynamicContext dynamic_context{relative_dynamic_context()};
  Instructions compiler_template;

  for (const auto &destination : context.precompiled_static_schemas) {
    assert(context.frame.locations().contains(
        {sourcemeta::core::SchemaReferenceType::Static, destination}));
    const auto match{context.frame.locations().find(
        {sourcemeta::core::SchemaReferenceType::Static, destination})};
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
    for (const auto &entry : context.frame.locations()) {
      // We are only trying to find dynamic anchors
      if (entry.second.type !=
              sourcemeta::core::SchemaFrame::LocationType::Anchor ||
          entry.first.first != sourcemeta::core::SchemaReferenceType::Dynamic) {
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

  const bool track{
      context.mode != Mode::FastValidation ||
      requires_evaluation(context, schema_context) ||
      // TODO: This expression should go away if we start properly compiling
      // `unevaluatedItems` like we compile `unevaluatedProperties`
      std::ranges::any_of(context.unevaluated, [](const auto &dependency) {
        return dependency.first.ends_with("unevaluatedItems");
      })};
  if (compiler_template.empty()) {
    return {.instructions = std::move(children),
            .dynamic = uses_dynamic_scopes,
            .track = track};
  } else {
    compiler_template.reserve(compiler_template.size() + children.size());
    std::ranges::move(children, std::back_inserter(compiler_template));
    return {.instructions = std::move(compiler_template),
            .dynamic = uses_dynamic_scopes,
            .track = track};
  }
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler, const Mode mode,
             const std::optional<std::string> &default_dialect,
             const std::optional<std::string> &default_id) -> Template {
  assert(is_schema(schema));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here
  const sourcemeta::core::JSON result{sourcemeta::core::bundle(
      schema, walker, resolver, default_dialect, default_id)};

  // Perform framing to resolve references later on
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(result, walker, resolver, default_dialect, default_id);

  return compile(result, walker, resolver, compiler, frame, mode,
                 default_dialect, default_id);
}

auto compile(const Context &context, const SchemaContext &schema_context,
             const DynamicContext &dynamic_context,
             const sourcemeta::core::Pointer &schema_suffix,
             const sourcemeta::core::Pointer &instance_suffix,
             const std::optional<std::string> &uri) -> Instructions {
  // Determine URI of the destination after recursion
  const std::string destination{
      uri.has_value()
          ? sourcemeta::core::URI::canonicalize(uri.value())
          : to_uri(schema_context.relative_pointer.concat(schema_suffix),
                   schema_context.base)
                .canonicalize()
                .recompose()};

  // Otherwise the recursion attempt is non-sense
  if (!context.frame.locations().contains(
          {sourcemeta::core::SchemaReferenceType::Static, destination})) {
    throw sourcemeta::core::SchemaReferenceError(
        destination, schema_context.relative_pointer,
        "The target of the reference does not exist in the schema");
  }

  const auto &entry{context.frame.locations().at(
      {sourcemeta::core::SchemaReferenceType::Static, destination})};
  const auto &new_schema{get(context.root, entry.pointer)};

  if (!is_schema(new_schema)) {
    throw sourcemeta::core::SchemaReferenceError(
        destination, schema_context.relative_pointer,
        "The target of the reference is not a valid schema");
  }

  const sourcemeta::core::Pointer destination_pointer{
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location.concat(schema_suffix)
          : dynamic_context.base_schema_location
                .concat({dynamic_context.keyword})
                .concat(schema_suffix)};

  return compile_subschema(
      context,
      {.relative_pointer = entry.relative_pointer,
       .schema = new_schema,
       .vocabularies =
           vocabularies(new_schema, context.resolver, entry.dialect),
       .base = sourcemeta::core::URI{entry.base}
                   .recompose_without_fragment()
                   .value_or(""),
       // TODO: This represents a copy
       .labels = schema_context.labels,
       .references = schema_context.references,
       .is_property_name = schema_context.is_property_name},
      {.keyword = dynamic_context.keyword,
       .base_schema_location = destination_pointer,
       .base_instance_location =
           dynamic_context.base_instance_location.concat(instance_suffix),
       .property_as_target = dynamic_context.property_as_target},
      entry.dialect);
}

} // namespace sourcemeta::blaze
