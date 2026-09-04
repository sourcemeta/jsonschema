#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/foundation.h>

#include <algorithm> // std::move, std::sort, std::unique
#include <cassert>   // assert
#include <format>    // std::format
// TODO(C++23): Consider std::flat_map/std::flat_set when available in libc++
#include <map>           // std::map
#include <set>           // std::set
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <variant>       // std::holds_alternative
#include <vector>        // std::vector

#include "compile_helpers.h"
#include "keyword_iterator.h"
#include "postprocess.h"

namespace {

// A `$schema` reference points at a meta-schema rather than into the schema
// itself, so reference planning ignores it
auto is_metaschema_reference(const sourcemeta::core::WeakPointer &origin)
    -> bool {
  return !origin.empty() && origin.back().is_property() &&
         origin.back().to_property() == "$schema";
}

// Draft 6 introduced boolean schemas. Draft 4 and earlier have none, and the
// only places they accept a boolean are `additionalProperties` and
// `additionalItems`, whose own definitions spell that out
auto booleans_are_schemas(
    const sourcemeta::blaze::SchemaVocabularies &vocabularies) -> bool {
  using Known = sourcemeta::blaze::SchemaVocabularies::Known;
  return !vocabularies.contains_any(
      {Known::JSON_Schema_Draft_3, Known::JSON_Schema_Draft_3_Hyper,
       Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper});
}

// Draft 4 and earlier spell these as flags on a sibling bound rather than as
// bounds of their own, and their meta-schemas ask for that sibling to be there
auto exclusive_bounds_need_a_sibling(
    const sourcemeta::blaze::SchemaVocabularies &vocabularies) -> bool {
  using Known = sourcemeta::blaze::SchemaVocabularies::Known;
  return vocabularies.contains_any(
      {Known::JSON_Schema_Draft_3, Known::JSON_Schema_Draft_3_Hyper,
       Known::JSON_Schema_Draft_4, Known::JSON_Schema_Draft_4_Hyper});
}

auto is_schema(const sourcemeta::core::JSON &value, const bool allow_boolean)
    -> bool {
  return value.is_object() || (allow_boolean && value.is_boolean());
}

// The reason the keyword's value is not the shape the meta-schema asks for, or
// `nullptr` when it is. The location the error carries names the keyword, so
// these read as statements about it
auto keyword_shape_error(
    const sourcemeta::core::JSON::String &keyword,
    const sourcemeta::blaze::SchemaKeywordType type, const bool known,
    const sourcemeta::core::JSON &value, const bool allow_boolean,
    const sourcemeta::blaze::SchemaVocabularies &vocabularies) -> const char * {
  using namespace sourcemeta::blaze;
  static constexpr auto EXPECTED_STRING{
      "This keyword was expected to be set to a string"};
  static constexpr auto EXPECTED_ARRAY{
      "This keyword was expected to be set to an array"};
  static constexpr auto EXPECTED_BOOLEAN{
      "This keyword was expected to be set to a boolean"};
  static constexpr auto EXPECTED_NUMBER{
      "This keyword was expected to be set to a number"};
  static constexpr auto EXPECTED_NON_NEGATIVE_INTEGER{
      "This keyword was expected to be set to a non-negative integer"};
  static constexpr auto EXPECTED_SCHEMA{
      "This keyword was expected to be set to a valid schema"};
  static constexpr auto EXPECTED_SCHEMA_OR_ARRAY{
      "This keyword was expected to be set to a valid schema or to an array "
      "of them"};
  static constexpr auto EXPECTED_SCHEMA_ARRAY{
      "This keyword was expected to be set to an array of valid schemas"};
  static constexpr auto EXPECTED_SCHEMA_OBJECT{
      "This keyword was expected to be set to an object whose values are "
      "valid schemas"};

  // What follows describes the contracts of the official vocabularies. A
  // keyword the dialect does not define carries no vocabulary at all, and one
  // that a custom vocabulary defines carries its URI rather than a known
  // value, so neither is held to these
  if (known) {
    if (keyword == "title" || keyword == "description" ||
        keyword == "$comment" || keyword == "format" ||
        keyword == "contentEncoding" || keyword == "contentMediaType") {
      return value.is_string() ? nullptr : EXPECTED_STRING;
    } else if (keyword == "uniqueItems" || keyword == "deprecated" ||
               keyword == "readOnly" || keyword == "writeOnly") {
      return value.is_boolean() ? nullptr : EXPECTED_BOOLEAN;
    } else if (keyword == "examples") {
      return value.is_array() ? nullptr : EXPECTED_ARRAY;
    } else if (keyword == "maxContains" || keyword == "minContains") {
      // These only exist from 2019-09 onwards, where a number whose fractional
      // part is zero counts as an integer
      return (value.is_integral() && value.is_positive())
                 ? nullptr
                 : EXPECTED_NON_NEGATIVE_INTEGER;
    } else if (keyword == "exclusiveMaximum" || keyword == "exclusiveMinimum") {
      return (vocabularies.contains_any(
                  {SchemaVocabularies::Known::JSON_Schema_Draft_3,
                   SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
                   SchemaVocabularies::Known::JSON_Schema_Draft_4,
                   SchemaVocabularies::Known::JSON_Schema_Draft_4_Hyper})
                  ? (value.is_boolean() ? nullptr : EXPECTED_BOOLEAN)
                  : (value.is_number() ? nullptr : EXPECTED_NUMBER));
    } else if ((keyword == "$defs" || keyword == "definitions") &&
               // The walker treats these as containers in every dialect, but
               // no meta-schema before Draft 4 defines either of them
               !vocabularies.contains_any(
                   {SchemaVocabularies::Known::JSON_Schema_Draft_3,
                    SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper})) {
      return (value.is_object() &&
              std::ranges::all_of(value.as_object(),
                                  [allow_boolean](const auto &entry) -> bool {
                                    return is_schema(entry.second,
                                                     allow_boolean);
                                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_OBJECT;
    }
  }

  // Draft 3 spells `required` as a flag on the property itself rather than as
  // a list on the object, so the shape it asks for is a different one
  if (keyword == "required" &&
      vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_Schema_Draft_3,
           SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper})) {
    return value.is_boolean() ? nullptr : EXPECTED_BOOLEAN;
  }

  switch (type) {
    case SchemaKeywordType::ApplicatorValueTraverseSomeProperty:
    case SchemaKeywordType::ApplicatorValueTraverseAnyPropertyKey:
    case SchemaKeywordType::ApplicatorValueTraverseAnyItem:
    case SchemaKeywordType::ApplicatorValueTraverseSomeItem:
    case SchemaKeywordType::ApplicatorValueTraverseParent:
    case SchemaKeywordType::ApplicatorValueInPlaceMaybe:
    case SchemaKeywordType::ApplicatorValueInPlaceOther:
    case SchemaKeywordType::ApplicatorValueInPlaceNegate:
      return is_schema(value, allow_boolean) ? nullptr : EXPECTED_SCHEMA;
    case SchemaKeywordType::ApplicatorValueOrElementsTraverseAnyItemOrItem:
    case SchemaKeywordType::ApplicatorValueOrElementsInPlace:
      return (is_schema(value, allow_boolean) || value.is_array())
                 ? nullptr
                 : EXPECTED_SCHEMA_OR_ARRAY;
    // Note that `ApplicatorElementsInPlaceSome` and its negated variant are
    // deliberately absent, as Draft 3 `type` and `disallow` also take a plain
    // type name, so their strategy does not mandate an array. So is
    // `ApplicatorMembersInPlaceSome`, as Draft 4 `dependencies` also takes a
    // list of property names as a member
    case SchemaKeywordType::ApplicatorElementsTraverseItem:
    case SchemaKeywordType::ApplicatorElementsInPlace:
      return (value.is_array() &&
              std::ranges::all_of(value.as_array(),
                                  [allow_boolean](const auto &entry) -> bool {
                                    return is_schema(entry, allow_boolean);
                                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_ARRAY;
    case SchemaKeywordType::ApplicatorMembersTraversePropertyStatic:
    case SchemaKeywordType::ApplicatorMembersTraversePropertyRegex:
      return (value.is_object() &&
              std::ranges::all_of(value.as_object(),
                                  [allow_boolean](const auto &entry) -> bool {
                                    return is_schema(entry.second,
                                                     allow_boolean);
                                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_OBJECT;
    // The walker only reports this for a keyword the dialect in use actually
    // defines, so an unknown keyword never reaches here and stays ignored, as
    // the specification requires
    case SchemaKeywordType::Annotation:
      if (keyword == "title" || keyword == "description" ||
          keyword == "contentEncoding" || keyword == "contentMediaType") {
        return value.is_string() ? nullptr : EXPECTED_STRING;
      } else if (keyword == "examples") {
        return value.is_array() ? nullptr : EXPECTED_ARRAY;
      } else if (keyword == "deprecated" || keyword == "readOnly" ||
                 keyword == "writeOnly") {
        return value.is_boolean() ? nullptr : EXPECTED_BOOLEAN;
      }

      // Others, like `default`, take any value at all
      return nullptr;
    default:
      return nullptr;
  }
}

auto compile_subschema(const sourcemeta::blaze::Context &context,
                       const sourcemeta::blaze::SchemaContext &schema_context,
                       const sourcemeta::blaze::DynamicContext &dynamic_context)
    -> sourcemeta::blaze::Instructions {
  using namespace sourcemeta::blaze;
  assert((schema_context.schema.is_object() ||
          schema_context.schema.is_boolean()));

  // A boolean in a keyword position is settled by the keyword's own contract,
  // which the shape check below applies. What is left is the root of a schema
  // resource, where a dialect without boolean schemas admits no boolean at all
  if (schema_context.schema.is_boolean() &&
      schema_context.relative_pointer.empty() &&
      !booleans_are_schemas(schema_context.vocabularies)) [[unlikely]] {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        "This dialect does not support boolean schemas");
  }

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
  for (const auto &entry : sourcemeta::blaze::SchemaKeywordIterator{
           schema_context.schema, context.walker,
           schema_context.vocabularies}) {
    assert(entry.pointer.back().is_property());
    const auto &keyword{entry.pointer.back().to_property()};
    const auto &metadata{context.walker(keyword, schema_context.vocabularies)};
    const auto official{
        metadata.vocabulary.has_value() &&
        std::holds_alternative<sourcemeta::blaze::SchemaVocabularies::Known>(
            metadata.vocabulary.value())};
    // Draft 3 has no boolean schemas, but its own definitions of these two
    // keywords accept a boolean in place of one
    const auto allow_boolean{
        booleans_are_schemas(schema_context.vocabularies) ||
        keyword == "additionalProperties" || keyword == "additionalItems"};
    const auto *shape_error{keyword_shape_error(
        keyword, metadata.type, official, schema_context.schema.at(keyword),
        allow_boolean, schema_context.vocabularies)};
    // Draft 3 spells these as flags on a sibling bound rather than as bounds of
    // their own, and its meta-schema asks for that sibling to be there
    static const sourcemeta::core::JSON::String KEYWORD_MINIMUM{"minimum"};
    static const sourcemeta::core::JSON::String KEYWORD_MAXIMUM{"maximum"};
    if (official &&
        exclusive_bounds_need_a_sibling(schema_context.vocabularies) &&
        ((keyword == "exclusiveMinimum" &&
          !schema_context.schema.defines(KEYWORD_MINIMUM)) ||
         (keyword == "exclusiveMaximum" &&
          !schema_context.schema.defines(KEYWORD_MAXIMUM)))) [[unlikely]] {
      throw sourcemeta::blaze::CompilerError(
          schema_context.base,
          absolute_schema_location(
              context, schema_context.base,
              schema_context.relative_pointer.concat(
                  sourcemeta::blaze::make_weak_pointer(keyword))),
          "This keyword was expected to accompany the bound it applies to");
    }

    if (shape_error) [[unlikely]] {
      throw sourcemeta::blaze::CompilerError(
          schema_context.base,
          absolute_schema_location(
              context, schema_context.base,
              schema_context.relative_pointer.concat(
                  sourcemeta::blaze::make_weak_pointer(keyword))),
          shape_error);
    }

    // Bases must not contain fragments
    assert(!schema_context.base.fragment().has_value());
    for (auto &&step : context.compiler(
             context,
             {.relative_pointer = schema_context.relative_pointer.concat(
                  make_weak_pointer(keyword)),
              .schema = schema_context.schema,
              .vocabularies = schema_context.vocabularies,
              .base = schema_context.base,
              .is_property_name = schema_context.is_property_name},
             {.keyword = keyword,
              .base_schema_location = dynamic_context.base_schema_location,
              .base_instance_location = dynamic_context.base_instance_location},
             steps)) {
      // Just a sanity check to ensure every keyword location is indeed valid
      assert(sourcemeta::core::try_get(
                 context.root,
                 absolute_schema_pointer(context, schema_context)) != nullptr);
      steps.push_back(std::move(step));
    }
  }

  return steps;
}

auto defines_any_whitelisted_keyword(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaFrame &frame,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const sourcemeta::blaze::SchemaFrame::Location &entrypoint_location,
    const std::unordered_set<sourcemeta::core::JSON::StringView> &keywords)
    -> bool {
  std::vector<std::pair<sourcemeta::core::JSON::StringView,
                        sourcemeta::core::JSON::Object::hash_type>>
      hashed_keywords;
  hashed_keywords.reserve(keywords.size());
  for (const auto &keyword : keywords) {
    hashed_keywords.emplace_back(keyword,
                                 sourcemeta::core::JSON::Object::hash(keyword));
  }

  return frame.any_subschema(
      [&](const sourcemeta::blaze::SchemaFrame::Location &location) -> bool {
        const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
        if (!subschema.is_object()) {
          return false;
        }

        bool defines_keyword{false};
        for (const auto &[keyword, keyword_hash] : hashed_keywords) {
          if (subschema.defines(keyword, keyword_hash)) {
            defines_keyword = true;
            break;
          }
        }

        return defines_keyword &&
               frame.is_reachable(entrypoint_location, location, walker,
                                  resolver);
      });
}

// TODO: Somehow move this logic up to `SchemaFrame`
auto schema_frame_populate_target_types(
    const sourcemeta::blaze::SchemaFrame &frame,
    std::unordered_map<std::string_view, std::pair<bool, bool>> &target_types)
    -> void {
  frame.for_each_reference(
      [&](const sourcemeta::blaze::SchemaReferenceType,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::blaze::SchemaFrame::Reference &reference) -> void {
        if (is_metaschema_reference(origin)) {
          return;
        }

        // A reference is always a keyword of the subschema that declares it,
        // so that subschema is what the origin sits directly inside of
        const auto reference_location{frame.traverse(origin.initial())};
        assert(reference_location.has_value());
        auto &context{target_types[reference.destination]};
        if (reference_location->get().property_name) {
          context.first = true;
        } else {
          context.second = true;
        }
      });

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
  frame.for_each_reference(
      [&](const sourcemeta::blaze::SchemaReferenceType,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::blaze::SchemaFrame::Reference &reference) -> void {
        if (is_metaschema_reference(origin)) {
          return;
        }

        for (const auto &[destination, destination_pointer] :
             destination_pointers) {
          if (origin.starts_with(*destination_pointer) &&
              origin.size() > destination_pointer->size()) {
            references_within[destination].push_back(reference.destination);
          }
        }
      });

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

// Whether an entry point that the frame could not locate nonetheless names a
// place that the document has, which means it is a keyword rather than a
// schema. Only reachable when the entry point carries a pointer fragment, as
// anything else can only ever name a schema
auto entrypoint_names_non_schema(const sourcemeta::core::JSON &schema,
                                 const sourcemeta::blaze::SchemaFrame &frame,
                                 const std::string_view entrypoint) -> bool {
  std::optional<sourcemeta::core::URI> uri;
  try {
    uri.emplace(sourcemeta::core::JSON::String{entrypoint});
  } catch (const sourcemeta::core::URIParseError &) {
    // A URI that does not even parse names nothing at all, so leave the
    // caller to report it as the invalid entry point that it is
    return false;
  }

  const auto relative{sourcemeta::core::fragment_to_pointer(uri.value())};
  if (!relative.has_value()) {
    return false;
  }

  const auto base{uri.value().recompose_without_fragment().value_or(
      sourcemeta::core::JSON::String{})};
  const auto base_location{frame.traverse(base)};
  if (!base_location.has_value()) {
    // A base that framing does not know names some other document, which this
    // one has nothing to say about
    return base.empty() &&
           sourcemeta::core::try_get(schema, relative.value()) != nullptr;
  }

  return sourcemeta::core::try_get(
             schema,
             sourcemeta::core::to_pointer(base_location.value().get().pointer)
                 .concat(relative.value())) != nullptr;
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::blaze::SchemaWalker &walker,
             const sourcemeta::blaze::SchemaResolver &resolver,
             const Compiler &compiler,
             const sourcemeta::blaze::SchemaFrame &frame,
             const std::string_view entrypoint, const Mode mode,
             const std::optional<Tweaks> &tweaks) -> Template {
  assert((schema.is_object() || schema.is_boolean()));
  const auto effective_tweaks{tweaks.value_or(Tweaks{})};

  const auto maybe_entrypoint_location{frame.traverse(entrypoint)};
  if (!maybe_entrypoint_location.has_value()) [[unlikely]] {
    // A frame that only locates schemas has nothing to say about an entry
    // point that names a plain keyword, so tell the two apart by asking the
    // document rather than reporting a place that does exist as missing
    throw CompilerInvalidEntryPoint{
        entrypoint,
        entrypoint_names_non_schema(schema, frame, entrypoint)
            ? "The given entry point URI is not a valid subschema"
            : "The given entry point URI does not exist in the schema"};
  }

  const auto &entrypoint_location{maybe_entrypoint_location->get()};
  if (entrypoint_location.type ==
      sourcemeta::blaze::SchemaFrame::LocationType::Pointer) [[unlikely]] {
    throw CompilerInvalidEntryPoint{
        entrypoint, "The given entry point URI is not a valid subschema"};
  }

  // Compiling from an entry point makes that subschema a root of its own, and
  // a dialect without boolean schemas has no more room for a boolean there
  // than it has at the root of a schema resource
  const auto &entrypoint_schema{
      sourcemeta::core::get(schema, entrypoint_location.pointer)};
  if (entrypoint_schema.is_boolean() &&
      !booleans_are_schemas(frame.vocabularies(entrypoint_location, resolver)))
      [[unlikely]] {
    throw CompilerError(sourcemeta::core::URI{entrypoint_location.base},
                        to_pointer(entrypoint_location.pointer),
                        "This dialect does not support boolean schemas");
  }

  const bool collects_annotations{
      mode == Mode::Exhaustive ||
      (effective_tweaks.annotations.has_value() &&
       !effective_tweaks.annotations.value().empty() &&
       defines_any_whitelisted_keyword(schema, frame, walker, resolver,
                                       entrypoint_location,
                                       effective_tweaks.annotations.value()))};

  ///////////////////////////////////////////////////////////////////
  // (1) Determine all the schema resources in the schema
  ///////////////////////////////////////////////////////////////////

  std::vector<std::string> resources;
  frame.for_each_resource(
      [&resources](const std::string_view uri,
                   const sourcemeta::blaze::SchemaFrame::Location &) -> void {
        resources.emplace_back(uri);
      });

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

  // If dynamic referencing does not take place in this schema, we can avoid
  // the overhead of keeping track of dynamic scopes, etc
  const auto uses_dynamic_scopes{frame.has_dynamic_references()};

  ///////////////////////////////////////////////////////////////////
  // (3) Plan which static references we will precompile
  ///////////////////////////////////////////////////////////////////

  std::unordered_map<std::string_view, std::pair<bool, bool>> target_types;
  schema_frame_populate_target_types(frame, target_types);

  std::map<std::tuple<sourcemeta::blaze::SchemaReferenceType, std::string_view,
                      bool>,
           std::pair<std::size_t, const sourcemeta::core::WeakPointer *>>
      targets_map;
  targets_map.emplace(
      std::make_tuple(sourcemeta::blaze::SchemaReferenceType::Static,
                      entrypoint, false),
      std::make_pair(0, nullptr));

  frame.for_each_reference(
      [&](const sourcemeta::blaze::SchemaReferenceType type,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::blaze::SchemaFrame::Reference &reference) -> void {
        if (is_metaschema_reference(origin)) {
          return;
        }

        const auto reference_origin{frame.traverse(origin.initial())};
        assert(reference_origin.has_value());

        // Skip unreachable targets
        if (!frame.is_reachable(entrypoint_location, reference_origin->get(),
                                walker, resolver)) {
          return;
        }

        assert(target_types.contains(reference.destination));
        const auto &[needs_name,
                     needs_instance]{target_types.at(reference.destination)};

        if (needs_name) {
          targets_map.emplace(
              std::make_tuple(type, std::string_view{reference.destination},
                              true),
              std::make_pair(targets_map.size(), &origin));
        }

        if (needs_instance) {
          targets_map.emplace(
              std::make_tuple(type, std::string_view{reference.destination},
                              false),
              std::make_pair(targets_map.size(), &origin));
        }
      });

  // Also add dynamic anchors that may not be directly referenced
  // but could be used as override targets during dynamic resolution
  frame.for_each_anchor(
      sourcemeta::blaze::SchemaReferenceType::Dynamic,
      [&](const std::string_view uri,
          const sourcemeta::blaze::SchemaFrame::Location &location) -> void {
        // Skip unreachable dynamic anchors
        if (!frame.is_reachable(entrypoint_location, location, walker,
                                resolver)) {
          return;
        }

        targets_map.emplace(
            std::make_tuple(sourcemeta::blaze::SchemaReferenceType::Dynamic,
                            uri, false),
            std::make_pair(targets_map.size(), nullptr));
      });

  ///////////////////////////////////////////////////////////////////
  // (4) Build the global compilation context
  ///////////////////////////////////////////////////////////////////

  auto unevaluated{
      sourcemeta::blaze::unevaluated(schema, frame, walker, resolver)};

  std::vector<InstructionExtra> instruction_extra;
  std::vector<SchemaVocabularies::URI> instruction_vocabularies;
  const Context context{.root = schema,
                        .frame = frame,
                        .resources = std::move(resources),
                        .walker = walker,
                        .resolver = resolver,
                        .compiler = compiler,
                        .mode = mode,
                        .uses_dynamic_scopes = uses_dynamic_scopes,
                        .collects_annotations = collects_annotations,
                        .unevaluated = std::move(unevaluated),
                        .tweaks = effective_tweaks,
                        .targets = std::move(targets_map),
                        .extra = instruction_extra,
                        .vocabularies = instruction_vocabularies};

  ///////////////////////////////////////////////////////////////////
  // (5) Build labels map for dynamic anchors
  ///////////////////////////////////////////////////////////////////

  std::vector<std::pair<std::size_t, std::size_t>> labels_map;
  if (uses_dynamic_scopes) {
    context.frame.for_each_anchor(
        sourcemeta::blaze::SchemaReferenceType::Dynamic,
        [&](const std::string_view uri,
            const sourcemeta::blaze::SchemaFrame::Location &entry) -> void {
          // Skip unreachable dynamic anchors
          if (!context.frame.is_reachable(entrypoint_location, entry,
                                          context.walker, context.resolver)) {
            return;
          }

          // Compute the hash for this dynamic anchor
          const sourcemeta::core::URI anchor_uri{uri};
          const auto label{Evaluator::hash(
              schema_resource_id(
                  context.resources,
                  anchor_uri.recompose_without_fragment().value_or("")),
              anchor_uri.fragment().value_or(""))};

          // Find the index in targets for this dynamic anchor
          const auto key{
              std::make_tuple(sourcemeta::blaze::SchemaReferenceType::Dynamic,
                              std::string_view{uri}, false)};
          assert(context.targets.contains(key));
          const auto index{context.targets.at(key).first};
          assert(index < context.targets.size());

          labels_map.emplace_back(label, index);
        });
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
    if (!location.has_value()) [[unlikely]] {
      assert(reference_pointer != nullptr);
      throw CompilerReferenceTargetNotSchemaError(
          destination_uri, to_pointer(*reference_pointer));
    }

    const auto &entry{location->get()};

    if (entry.type != sourcemeta::blaze::SchemaFrame::LocationType::Subschema &&
        entry.type != sourcemeta::blaze::SchemaFrame::LocationType::Resource &&
        entry.type != sourcemeta::blaze::SchemaFrame::LocationType::Anchor)
        [[unlikely]] {
      assert(reference_pointer != nullptr);
      const auto parent_size{entry.parent ? entry.parent->size() : 0};
      throw CompilerReferenceTargetNotSchemaError(
          destination_uri,
          to_pointer(entry.pointer.slice(
              0, std::min(parent_size + 1, entry.pointer.size()))));
    }

    auto subschema{sourcemeta::core::get(context.root, entry.pointer)};
    auto nested_vocabularies{
        context.frame.vocabularies(entry, context.resolver)};
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
                sourcemeta::core::EMPTY_WEAK_POINTER,
                sourcemeta::core::EMPTY_WEAK_POINTER, destination_uri);
  }

  ///////////////////////////////////////////////////////////////////
  // (7) Postprocess compiled targets
  ///////////////////////////////////////////////////////////////////

  if (mode == Mode::FastValidation) {
    postprocess(compiled_targets, instruction_extra, effective_tweaks,
                uses_dynamic_scopes);
  }

  ///////////////////////////////////////////////////////////////////
  // (8) Return final template
  ///////////////////////////////////////////////////////////////////

  // The template stores vocabularies as plain strings, as the evaluator does
  // not otherwise depend on the schema machinery
  std::vector<std::string> template_vocabularies;
  template_vocabularies.reserve(instruction_vocabularies.size());
  for (const auto &vocabulary : instruction_vocabularies) {
    template_vocabularies.push_back(std::format("{}", vocabulary));
  }

  const bool track{
      context.mode != Mode::FastValidation ||
      requires_evaluation(context, entrypoint_location.pointer) ||
      // TODO: This expression should go away if we start properly compiling
      // `unevaluatedItems` like we compile `unevaluatedProperties`
      std::ranges::any_of(
          context.unevaluated, [](const auto &dependency) -> auto {
            return dependency.first.ends_with("unevaluatedItems");
          })};
  return {.dynamic = uses_dynamic_scopes,
          .track = track,
          .targets = std::move(compiled_targets),
          .labels = std::move(labels_map),
          .extra = std::move(instruction_extra),
          .vocabularies = std::move(template_vocabularies)};
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::blaze::SchemaWalker &walker,
             const sourcemeta::blaze::SchemaResolver &resolver,
             const Compiler &compiler, const Mode mode,
             const std::string_view default_dialect,
             const std::string_view default_id,
             const std::string_view entrypoint,
             const std::optional<Tweaks> &tweaks) -> Template {
  assert((schema.is_object() || schema.is_boolean()));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here. Meta-schemas are not needed, as we
  // can determine vocabularies through the resolver
  const sourcemeta::core::JSON result{sourcemeta::blaze::bundle(
      schema, walker, resolver, sourcemeta::blaze::BundleMode::References,
      default_dialect, default_id)};

  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References,
      result,
      walker,
      resolver,
      default_dialect,
      default_id};
  return compile(result, walker, resolver, compiler, frame,
                 entrypoint.empty() ? frame.root() : entrypoint, mode, tweaks);
}

auto compile(const Context &context, const SchemaContext &schema_context,
             const DynamicContext &dynamic_context,
             const sourcemeta::core::WeakPointer &schema_suffix,
             const sourcemeta::core::WeakPointer &instance_suffix,
             const std::optional<std::string_view> uri) -> Instructions {
  // An explicit URI is a jump elsewhere in the schema, which only the frame
  // can resolve. Without one we are recursing within the subschema we are
  // already at, so where we land follows from the pointer we came in with
  std::optional<
      std::reference_wrapper<const sourcemeta::blaze::SchemaFrame::Location>>
      entry;
  sourcemeta::core::WeakPointer target;
  if (uri.has_value()) {
    const auto destination{sourcemeta::core::URI::canonicalize(uri.value())};
    entry = context.frame.location(
        sourcemeta::blaze::SchemaReferenceType::Static, destination);
    // Otherwise the recursion attempt is non-sense
    if (!entry.has_value()) [[unlikely]] {
      throw sourcemeta::blaze::SchemaReferenceError(
          destination, absolute_schema_location(context, schema_context),
          "The target of the reference does not exist in the schema");
    }

    target = entry.value().get().pointer;
  } else {
    target =
        absolute_schema_pointer(context, schema_context).concat(schema_suffix);
    // Otherwise the recursion attempt is non-sense
    if (sourcemeta::core::try_get(context.root, target) == nullptr)
        [[unlikely]] {
      throw sourcemeta::blaze::SchemaReferenceError(
          to_uri(schema_context.relative_pointer.concat(schema_suffix),
                 schema_context.base)
              .canonicalize()
              .recompose(),
          absolute_schema_location(context, schema_context),
          "The target of the reference does not exist in the schema");
    }

    // A pointer that sits under more than one base is framed once per base, so
    // this may be any of those entries. They are interchangeable here, as
    // framing resolves the base, the dialect and the depth of a location from
    // the resource nearest to it rather than from the URI it got keyed under,
    // so every entry of a given pointer reports the same ones
    entry = context.frame.traverse(target);
  }

  const auto &new_schema{get(context.root, target)};

  // An invalid schema may set an applicator to a value that is not a schema
  // at all, in which case we consider it to impose no constraints
  if (!new_schema.is_object() && !new_schema.is_boolean()) [[unlikely]] {
    return {};
  }

  const sourcemeta::core::WeakPointer destination_pointer{
      dynamic_context.keyword.empty()
          ? dynamic_context.base_schema_location.concat(schema_suffix)
          : dynamic_context.base_schema_location
                .concat(make_weak_pointer(dynamic_context.keyword))
                .concat(schema_suffix)};

  // A schema that the walker never descended into has no location of its own,
  // in which case it inherits the resource and the dialect of what encloses it
  const auto new_relative_pointer{
      entry.has_value()
          ? entry.value().get().pointer.slice(
                entry.value().get().relative_pointer)
          : schema_context.relative_pointer.concat(schema_suffix)};
  const sourcemeta::core::URI new_base{
      entry.has_value()
          ? sourcemeta::core::URI{entry.value().get().base}
                .recompose_without_fragment()
                .value_or("")
          : schema_context.base.recompose_without_fragment().value_or("")};

  return compile_subschema(
      context,
      {.relative_pointer = new_relative_pointer,
       .schema = new_schema,
       .vocabularies = entry.has_value()
                           ? context.frame.vocabularies(entry.value().get(),
                                                        context.resolver)
                           : schema_context.vocabularies,
       .base = new_base,
       .is_property_name = schema_context.is_property_name},
      {.keyword = dynamic_context.keyword,
       .base_schema_location = destination_pointer,
       .base_instance_location =
           dynamic_context.base_instance_location.concat(instance_suffix)});
}

} // namespace sourcemeta::blaze
