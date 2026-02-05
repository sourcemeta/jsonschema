#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/jsonschema.h>

#include <algorithm>     // std::move, std::sort, std::unique
#include <cassert>       // assert
#include <map>           // std::map
#include <set>           // std::set
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

#include "compile_helpers.h"
#include "postprocess.h"

namespace {

auto compile_subschema(const sourcemeta::blaze::Context &context,
                       const sourcemeta::blaze::SchemaContext &schema_context,
                       const sourcemeta::blaze::DynamicContext &dynamic_context,
                       const std::string_view default_dialect)
    -> sourcemeta::blaze::Instructions {
  using namespace sourcemeta::blaze;
  assert(is_schema(schema_context.schema));

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (schema_context.schema.is_boolean()) {
    if (schema_context.schema.to_boolean()) {
      return {};
    } else {
      return {make(
          sourcemeta::blaze::InstructionIndex::AssertionFail, context,
          schema_context,
          {.keyword = KEYWORD_EMPTY,
           .base_schema_location = dynamic_context.base_schema_location,
           .base_instance_location = dynamic_context.base_instance_location},
          ValueNone{})};
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
             {.relative_pointer = schema_context.relative_pointer.concat(
                  make_weak_pointer(keyword)),
              .schema = schema_context.schema,
              .vocabularies = entry.vocabularies,
              .base = schema_context.base,
              .is_property_name = schema_context.is_property_name},
             {.keyword = keyword,
              .base_schema_location = dynamic_context.base_schema_location,
              .base_instance_location = dynamic_context.base_instance_location},
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

// TODO: Somehow move this logic up to `SchemaFrame`
auto schema_frame_populate_target_types(
    const sourcemeta::core::SchemaFrame &frame,
    std::unordered_map<std::string_view, std::pair<bool, bool>> &target_types)
    -> void {
  for (const auto &reference : frame.references()) {
    if (!reference.first.second.empty() &&
        reference.first.second.back().is_property() &&
        reference.first.second.back().to_property() == "$schema") {
      continue;
    }

    const auto reference_location{frame.traverse(reference.first.second)};
    assert(reference_location.has_value());
    auto &context{target_types[reference.second.destination]};
    if (reference_location->get().property_name) {
      context.first = true;
    } else {
      context.second = true;
    }
  }

  std::unordered_map<std::string_view, const sourcemeta::core::WeakPointer *>
      destination_pointers;
  for (const auto &[destination, _] : target_types) {
    const auto destination_location{frame.traverse(destination)};
    if (destination_location.has_value()) {
      destination_pointers.emplace(destination,
                                   &destination_location->get().pointer);
    }
  }

  std::unordered_map<std::string_view, std::vector<std::string_view>>
      references_within;
  for (const auto &reference : frame.references()) {
    if (!reference.first.second.empty() &&
        reference.first.second.back().is_property() &&
        reference.first.second.back().to_property() == "$schema") {
      continue;
    }

    for (const auto &[destination, destination_pointer] :
         destination_pointers) {
      if (reference.first.second.starts_with(*destination_pointer) &&
          reference.first.second.size() > destination_pointer->size()) {
        references_within[destination].push_back(reference.second.destination);
      }
    }
  }

  bool changed{true};
  while (changed) {
    changed = false;
    for (const auto &[current_destination, context] : target_types) {
      if (!context.first) {
        continue;
      }

      const auto iterator{references_within.find(current_destination)};
      if (iterator == references_within.end()) {
        continue;
      }

      for (const auto &referenced_destination : iterator->second) {
        auto &next_context{target_types[referenced_destination]};
        if (!next_context.first) {
          next_context.first = true;
          changed = true;
        }
      }
    }
  }
}

} // namespace

namespace sourcemeta::blaze {

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const sourcemeta::core::SchemaFrame &frame,
             const std::string_view entrypoint, const Mode mode,
             const std::optional<Tweaks> &tweaks) -> Template {
  assert(is_schema(schema));
  const auto effective_tweaks{tweaks.value_or(Tweaks{})};

  const auto maybe_entrypoint_location{frame.traverse(entrypoint)};
  if (!maybe_entrypoint_location.has_value()) {
    throw CompilerInvalidEntryPoint{
        entrypoint, "The given entry point URI does not exist in the schema"};
  }

  const auto &entrypoint_location{maybe_entrypoint_location->get()};
  if (entrypoint_location.type ==
      sourcemeta::core::SchemaFrame::LocationType::Pointer) {
    throw CompilerInvalidEntryPoint{
        entrypoint, "The given entry point URI is not a valid subschema"};
  }

  ///////////////////////////////////////////////////////////////////
  // (1) Determine all the schema resources in the schema
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
  // (2) Check if the schema relies on dynamic scopes
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
  // (3) Plan which static references we will precompile
  ///////////////////////////////////////////////////////////////////

  std::unordered_map<std::string_view, std::pair<bool, bool>> target_types;
  schema_frame_populate_target_types(frame, target_types);

  std::map<
      std::tuple<sourcemeta::core::SchemaReferenceType, std::string_view, bool>,
      std::pair<std::size_t, const sourcemeta::core::WeakPointer *>>
      targets_map;
  targets_map.emplace(
      std::make_tuple(sourcemeta::core::SchemaReferenceType::Static, entrypoint,
                      false),
      std::make_pair(0, nullptr));

  for (const auto &reference : frame.references()) {
    // Ignore meta-schema references
    if (!reference.first.second.empty() &&
        reference.first.second.back().is_property() &&
        reference.first.second.back().to_property() == "$schema") {
      continue;
    }

    auto reference_origin{frame.traverse(reference.first.second)};
    assert(reference_origin.has_value());
    while (reference_origin->get().type ==
               sourcemeta::core::SchemaFrame::LocationType::Pointer &&
           reference_origin->get().parent.has_value()) {
      reference_origin = frame.traverse(reference_origin->get().parent.value());
      assert(reference_origin.has_value());
    }

    // Skip unreachable targets
    if (reference_origin->get().type !=
            sourcemeta::core::SchemaFrame::LocationType::Pointer &&
        !frame.is_reachable(entrypoint_location, reference_origin->get(),
                            walker, resolver)) {
      continue;
    }

    assert(target_types.contains(reference.second.destination));
    const auto &[needs_name,
                 needs_instance]{target_types.at(reference.second.destination)};

    if (needs_name) {
      targets_map.emplace(
          std::make_tuple(reference.first.first,
                          std::string_view{reference.second.destination}, true),
          std::make_pair(targets_map.size(), &reference.first.second));
    }

    if (needs_instance) {
      targets_map.emplace(
          std::make_tuple(reference.first.first,
                          std::string_view{reference.second.destination},
                          false),
          std::make_pair(targets_map.size(), &reference.first.second));
    }
  }

  // Also add dynamic anchors that may not be directly referenced
  // but could be used as override targets during dynamic resolution
  for (const auto &entry : frame.locations()) {
    if (entry.second.type !=
            sourcemeta::core::SchemaFrame::LocationType::Anchor ||
        entry.first.first != sourcemeta::core::SchemaReferenceType::Dynamic) {
      continue;
    }

    // Skip unreachable dynamic anchors
    if (!frame.is_reachable(entrypoint_location, entry.second, walker,
                            resolver)) {
      continue;
    }

    targets_map.emplace(std::make_tuple(entry.first.first,
                                        std::string_view{entry.first.second},
                                        false),
                        std::make_pair(targets_map.size(), nullptr));
  }

  ///////////////////////////////////////////////////////////////////
  // (4) Build the global compilation context
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
                        .tweaks = effective_tweaks,
                        .targets = std::move(targets_map)};

  ///////////////////////////////////////////////////////////////////
  // (5) Build labels map for dynamic anchors
  ///////////////////////////////////////////////////////////////////

  std::vector<std::pair<std::size_t, std::size_t>> labels_map;
  if (uses_dynamic_scopes) {
    for (const auto &entry : context.frame.locations()) {
      // We are only trying to find dynamic anchors
      if (entry.second.type !=
              sourcemeta::core::SchemaFrame::LocationType::Anchor ||
          entry.first.first != sourcemeta::core::SchemaReferenceType::Dynamic) {
        continue;
      }

      // Skip unreachable dynamic anchors
      if (!context.frame.is_reachable(entrypoint_location, entry.second,
                                      context.walker, context.resolver)) {
        continue;
      }

      // Compute the hash for this dynamic anchor
      const sourcemeta::core::URI anchor_uri{entry.first.second};
      const auto label{Evaluator::hash(
          schema_resource_id(
              context.resources,
              anchor_uri.recompose_without_fragment().value_or("")),
          anchor_uri.fragment().value_or(""))};

      // Find the index in targets for this dynamic anchor
      const auto key{
          std::make_tuple(sourcemeta::core::SchemaReferenceType::Dynamic,
                          std::string_view{entry.first.second}, false)};
      assert(context.targets.contains(key));
      const auto index{context.targets.at(key).first};
      assert(index < context.targets.size());

      labels_map.emplace_back(label, index);
    }
  }

  ///////////////////////////////////////////////////////////////////
  // (6) Compile targets for static references
  ///////////////////////////////////////////////////////////////////

  std::vector<Instructions> compiled_targets;
  compiled_targets.resize(context.targets.size());
  for (const auto &[destination, target_info] : context.targets) {
    const auto &[reference_type, destination_uri, is_property_name] =
        destination;
    const auto &[index, reference_pointer] = target_info;
    const auto location{context.frame.traverse(destination_uri)};
    assert(location.has_value());
    const auto &entry{location->get()};

    if (entry.type != sourcemeta::core::SchemaFrame::LocationType::Subschema &&
        entry.type != sourcemeta::core::SchemaFrame::LocationType::Resource &&
        entry.type != sourcemeta::core::SchemaFrame::LocationType::Anchor) {
      assert(reference_pointer != nullptr);
      const auto parent_size{entry.parent ? entry.parent->size() : 0};
      throw CompilerReferenceTargetNotSchemaError(
          destination_uri,
          to_pointer(entry.pointer.slice(
              0, std::min(parent_size + 1, entry.pointer.size()))));
    }

    auto subschema{sourcemeta::core::get(context.root, entry.pointer)};
    auto nested_vocabularies{sourcemeta::core::vocabularies(
        subschema, context.resolver, entry.dialect)};
    const auto nested_relative_pointer{
        entry.pointer.slice(entry.relative_pointer)};
    const sourcemeta::core::URI nested_base{entry.base};

    const SchemaContext schema_context{
        .relative_pointer = nested_relative_pointer,
        .schema = std::move(subschema),
        .vocabularies = std::move(nested_vocabularies),
        .base = nested_base,
        .is_property_name = is_property_name};

    compiled_targets[index] =
        compile(context, schema_context, relative_dynamic_context(),
                sourcemeta::core::empty_weak_pointer,
                sourcemeta::core::empty_weak_pointer, destination_uri);
  }

  ///////////////////////////////////////////////////////////////////
  // (7) Postprocess compiled targets
  ///////////////////////////////////////////////////////////////////

  if (mode == Mode::FastValidation) {
    postprocess(compiled_targets, effective_tweaks, uses_dynamic_scopes);
  }

  ///////////////////////////////////////////////////////////////////
  // (8) Return final template
  ///////////////////////////////////////////////////////////////////

  const bool track{
      context.mode != Mode::FastValidation ||
      requires_evaluation(context, entrypoint_location.pointer) ||
      // TODO: This expression should go away if we start properly compiling
      // `unevaluatedItems` like we compile `unevaluatedProperties`
      std::ranges::any_of(context.unevaluated, [](const auto &dependency) {
        return dependency.first.ends_with("unevaluatedItems");
      })};
  return {.dynamic = uses_dynamic_scopes,
          .track = track,
          .targets = std::move(compiled_targets),
          .labels = std::move(labels_map)};
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler, const Mode mode,
             const std::string_view default_dialect,
             const std::string_view default_id,
             const std::string_view entrypoint,
             const std::optional<Tweaks> &tweaks) -> Template {
  assert(is_schema(schema));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here
  const sourcemeta::core::JSON result{sourcemeta::core::bundle(
      schema, walker, resolver, default_dialect, default_id)};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(result, walker, resolver, default_dialect, default_id);
  return compile(result, walker, resolver, compiler, frame,
                 entrypoint.empty() ? frame.root() : entrypoint, mode, tweaks);
}

auto compile(const Context &context, const SchemaContext &schema_context,
             const DynamicContext &dynamic_context,
             const sourcemeta::core::WeakPointer &schema_suffix,
             const sourcemeta::core::WeakPointer &instance_suffix,
             const std::optional<std::string_view> uri) -> Instructions {
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
        destination, to_pointer(schema_context.relative_pointer),
        "The target of the reference does not exist in the schema");
  }

  const auto &entry{context.frame.locations().at(
      {sourcemeta::core::SchemaReferenceType::Static, destination})};
  const auto &new_schema{get(context.root, entry.pointer)};
  assert(is_schema(new_schema));

  const sourcemeta::core::WeakPointer destination_pointer{
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location.concat(schema_suffix)
          : dynamic_context.base_schema_location
                .concat(make_weak_pointer(dynamic_context.keyword))
                .concat(schema_suffix)};

  const auto new_relative_pointer{entry.pointer.slice(entry.relative_pointer)};
  const sourcemeta::core::URI new_base{
      sourcemeta::core::URI{entry.base}.recompose_without_fragment().value_or(
          "")};

  return compile_subschema(
      context,
      {.relative_pointer = new_relative_pointer,
       .schema = new_schema,
       .vocabularies =
           vocabularies(new_schema, context.resolver, entry.dialect),
       .base = new_base,
       .is_property_name = schema_context.is_property_name},
      {.keyword = dynamic_context.keyword,
       .base_schema_location = destination_pointer,
       .base_instance_location =
           dynamic_context.base_instance_location.concat(instance_suffix)},
      entry.dialect);
}

} // namespace sourcemeta::blaze
