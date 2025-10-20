#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::move, std::sort, std::unique
#include <cassert>   // assert
#include <iterator>  // std::back_inserter
#include <tuple>     // std::tuple, std::get
#include <utility>   // std::move, std::pair

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
      sourcemeta::blaze::schema_resource_id(
          context.resources,
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
             const std::optional<std::string> &default_id,
             const std::optional<Tweaks> &tweaks) -> Template {
  assert(is_schema(schema));
  const auto effective_tweaks{tweaks.value_or(Tweaks{})};

  ///////////////////////////////////////////////////////////////////
  // (1) Determine the root frame entry
  ///////////////////////////////////////////////////////////////////

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

  ///////////////////////////////////////////////////////////////////
  // (2) Determine all the schema resources in the schema
  ///////////////////////////////////////////////////////////////////

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

  ///////////////////////////////////////////////////////////////////
  // (3) Check if the schema relies on dynamic scopes
  ///////////////////////////////////////////////////////////////////

  bool uses_dynamic_scopes{false};
  for (const auto &reference : frame.references()) {
    // Check whether dynamic referencing takes places in this schema. If not,
    // we can avoid the overhead of keeping track of dynamics scopes, etc
    if (reference.first.first ==
        sourcemeta::core::SchemaReferenceType::Dynamic) {
      uses_dynamic_scopes = true;
      break;
    }
  }

  ///////////////////////////////////////////////////////////////////
  // (4) Plan which static references we will precompile
  ///////////////////////////////////////////////////////////////////

  // Use string views to avoid copying the actual strings, as we know
  // that the frame survives the entire compilation process
  std::vector<std::tuple<std::string_view, std::size_t, std::size_t>>
      sorted_precompile_references;

  if (effective_tweaks.precompile_static_references_maximum_schemas > 0) {
    std::unordered_map<std::string_view, std::pair<std::size_t, std::size_t>>
        static_reference_destinations;
    for (const auto &reference : frame.references()) {
      if (reference.first.first ==
              sourcemeta::core::SchemaReferenceType::Static &&
          frame.locations().contains(
              {sourcemeta::core::SchemaReferenceType::Static,
               reference.second.destination})) {
        std::unordered_set<std::string> visited;
        if (!effective_tweaks.precompile_static_references_non_circular &&
            !is_circular(frame, reference.first.second, reference.second,
                         visited)) {
          continue;
        }

        const auto label{Evaluator{}.hash(
            schema_resource_id(resources, reference.second.base.value_or("")),
            reference.second.fragment.value_or(""))};
        auto [iterator, inserted] = static_reference_destinations.try_emplace(
            reference.second.destination, std::make_pair(label, 0));
        iterator->second.second++;
      }
    }

    sorted_precompile_references.reserve(static_reference_destinations.size());
    for (const auto &reference : static_reference_destinations) {
      if (reference.second.second >=
          effective_tweaks
              .precompile_static_references_minimum_reference_count) {
        sorted_precompile_references.emplace_back(
            reference.first, reference.second.first, reference.second.second);
      }
    }
    std::ranges::sort(sorted_precompile_references,
                      [](const auto &left, const auto &right) {
                        return std::get<2>(left) > std::get<2>(right);
                      });

    if (sorted_precompile_references.size() >
        effective_tweaks.precompile_static_references_maximum_schemas) {
      sorted_precompile_references.erase(
          sorted_precompile_references.begin() +
              static_cast<std::ptrdiff_t>(
                  effective_tweaks
                      .precompile_static_references_maximum_schemas),
          sorted_precompile_references.end());
    }
  }

  assert(sorted_precompile_references.size() <=
         effective_tweaks.precompile_static_references_maximum_schemas);
  std::unordered_set<std::size_t> precompiled_labels;
  for (const auto &reference : sorted_precompile_references) {
    assert(
        std::get<2>(reference) >=
        effective_tweaks.precompile_static_references_minimum_reference_count);
    precompiled_labels.emplace(std::get<1>(reference));
  }

  ///////////////////////////////////////////////////////////////////
  // (5) Build the starting schema context
  ///////////////////////////////////////////////////////////////////

  SchemaContext schema_context{
      .relative_pointer = sourcemeta::core::empty_pointer,
      .schema = schema,
      .vocabularies = vocabularies(schema, resolver, root_frame_entry.dialect),
      .base = sourcemeta::core::URI::canonicalize(root_frame_entry.base),
      .labels = {},
      .is_property_name = false};

  ///////////////////////////////////////////////////////////////////
  // (6) Build the gloal compilation context
  ///////////////////////////////////////////////////////////////////

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
                        .precompiled_labels = std::move(precompiled_labels),
                        .tweaks = effective_tweaks};

  ///////////////////////////////////////////////////////////////////
  // (7) Build the initial dynamic context
  ///////////////////////////////////////////////////////////////////

  const DynamicContext dynamic_context{relative_dynamic_context()};

  ///////////////////////////////////////////////////////////////////
  // (8) Pre compile dynamic reference locations
  ///////////////////////////////////////////////////////////////////

  Instructions compiler_template;
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

  ///////////////////////////////////////////////////////////////////
  // (9) Pre compile static reference locations
  ///////////////////////////////////////////////////////////////////

  // Attempt to precompile static destinations to avoid explosive compilation
  Instructions static_reference_template;
  for (const auto &reference : sorted_precompile_references) {
    const auto entry{context.frame.locations().find(
        {sourcemeta::core::SchemaReferenceType::Static,
         std::string{std::get<0>(reference)}})};
    assert(entry != context.frame.locations().cend());
    auto subschema{sourcemeta::core::get(context.root, entry->second.pointer)};
    if (!sourcemeta::core::is_schema(subschema)) {
      continue;
    }

    auto nested_vocabularies{sourcemeta::core::vocabularies(
        subschema, context.resolver, entry->second.dialect)};
    const sourcemeta::blaze::SchemaContext nested_schema_context{
        .relative_pointer = entry->second.relative_pointer,
        .schema = std::move(subschema),
        .vocabularies = std::move(nested_vocabularies),
        // TODO: I think this is hiding a framing bug that we should later
        // investigate
        .base = entry->second.base.starts_with('#') ? "" : entry->second.base,
        .labels = {},
        .is_property_name = schema_context.is_property_name};
    static_reference_template.push_back(
        make(sourcemeta::blaze::InstructionIndex::ControlMark, context,
             nested_schema_context, dynamic_context,
             sourcemeta::blaze::ValueUnsignedInteger{std::get<1>(reference)},
             sourcemeta::blaze::compile(
                 context, nested_schema_context,
                 sourcemeta::blaze::relative_dynamic_context(),
                 sourcemeta::core::empty_pointer,
                 sourcemeta::core::empty_pointer, entry->first.second)));
  }

  for (auto &&substep : static_reference_template) {
    compiler_template.push_back(std::move(substep));
  }

  ///////////////////////////////////////////////////////////////////
  // (10) Compile the actual schema
  ///////////////////////////////////////////////////////////////////

  auto children{compile_subschema(context, schema_context, dynamic_context,
                                  root_frame_entry.dialect)};

  ///////////////////////////////////////////////////////////////////
  // (11) Return final template
  ///////////////////////////////////////////////////////////////////

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
             const std::optional<std::string> &default_id,
             const std::optional<Tweaks> &tweaks) -> Template {
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
                 default_dialect, default_id, tweaks);
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
       .is_property_name = schema_context.is_property_name},
      {.keyword = dynamic_context.keyword,
       .base_schema_location = destination_pointer,
       .base_instance_location =
           dynamic_context.base_instance_location.concat(instance_suffix),
       .property_as_target = dynamic_context.property_as_target},
      entry.dialect);
}

} // namespace sourcemeta::blaze
