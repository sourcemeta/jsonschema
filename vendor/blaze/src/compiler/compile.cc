#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/core/jsonschema.h>

#include <algorithm>  // std::ranges::sort, std::ranges::unique, std::min
#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <format>     // std::format
#include <functional> // std::cref, std::reference_wrapper
// TODO(C++23): Consider std::flat_map/std::flat_set when available in libc++
#include <map>           // std::map
#include <set>           // std::set
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <variant>       // std::holds_alternative
#include <vector>        // std::vector

#include "compile_helpers.h"
#include "message.h"
#include "postprocess.h"
#include "unevaluated.h"
#include "upstream.h"

namespace {

// Draft 7 and earlier let `$ref` override whatever sits beside it, so such a
// sibling is not a keyword of the schema at all and nothing about it, not even
// its shape, is this compiler's business
// Draft 4 and earlier spell these as flags on a sibling bound rather than as
// bounds of their own, and their meta-schemas ask for that sibling to be
// there. The OpenAPI 3.0 Schema Object is deliberately absent: it spells them
// as flags the way Draft 4 does, but its meta-schema declares no dependency
// between a flag and the bound it modifies, so one standing on its own is
// well-formed there and merely has nothing to modify
auto exclusive_bounds_need_a_sibling(
    const sourcemeta::core::SchemaVocabularies &vocabularies) -> bool {
  return sourcemeta::blaze::is_draft3(vocabularies) ||
         sourcemeta::blaze::is_draft4(vocabularies);
}

// The reason the keyword's value is not the shape the meta-schema asks for, or
// `nullptr` when it is. The location the error carries names the keyword, so
// these read as statements about it.
//
// What a dialect's meta-schema states is what settles this, rather than what
// its prose asks for, so that a schema the meta-schema accepts is one that
// compiles. A dialect whose prose carries a rule its meta-schema leaves
// unencoded therefore goes unenforced here: OpenAPI 3.0 asks that `items` be
// present whenever `type` is `"array"`, and that a Discriminator Object stand
// beside a composite keyword, while its meta-schema expresses neither, and it
// omits the lower bound that would keep `allOf`, `anyOf` and `oneOf` from
// standing empty. Catching those belongs to a linter, which is free to be
// stricter than the document the dialect publishes
auto keyword_shape_error(
    const sourcemeta::core::JSON::String &keyword,
    const sourcemeta::core::SchemaKeywordType type, const bool known,
    const sourcemeta::core::JSON &value,
    const sourcemeta::core::SchemaVocabularies &vocabularies) -> const char * {
  using namespace sourcemeta::blaze;

  // What follows describes the contracts of the official vocabularies. A
  // keyword the dialect does not define carries no vocabulary at all, and one
  // that a custom vocabulary defines carries its URI rather than a known
  // value, so neither is held to these
  if (known) {
    if (keyword == "title" || keyword == "description" ||
        keyword == "$comment" || keyword == "format" ||
        keyword == "contentEncoding" || keyword == "contentMediaType") {
      return value.is_string() ? nullptr : EXPECTED_STRING;
    }
    if (keyword == "uniqueItems" || keyword == "deprecated" ||
        keyword == "readOnly" || keyword == "writeOnly" ||
        keyword == "nullable") {
      return value.is_boolean() ? nullptr : EXPECTED_BOOLEAN;
    }
    if (keyword == "examples") {
      return value.is_array() ? nullptr : EXPECTED_ARRAY;
    }
    // The OpenAPI dialects define these three as objects of their own. What
    // follows is what their meta-schemas state, other than the `format`
    // annotations two of their members carry, which assert nothing by default
    if (keyword == "discriminator" || keyword == "xml" ||
        keyword == "externalDocs") {
      if (!value.is_object()) {
        return EXPECTED_OBJECT;
      }

      // Each OpenAPI version defines these objects for itself, and the later
      // ones carry members this does not know about, so only the dialect this
      // was written against is held to the shape below
      if (!vocabularies.contains(
              sourcemeta::core::SchemaVocabularies::Known::OPENAPI_3_0_BASE)) {
        return nullptr;
      }

      if (keyword == "discriminator") {
        const auto *property_name{value.try_at("propertyName")};
        if (property_name == nullptr || !property_name->is_string()) {
          return EXPECTED_DIALECT_OBJECT;
        }

        const auto *mapping{value.try_at("mapping")};
        if (mapping != nullptr &&
            (!mapping->is_object() ||
             std::ranges::any_of(mapping->as_object(),
                                 [](const auto &entry) -> bool {
                                   return !entry.second.is_string();
                                 }))) {
          return EXPECTED_DIALECT_OBJECT;
        }

        // Unlike the other two, this one its meta-schema leaves open
        return nullptr;
      }

      if (keyword == "externalDocs") {
        const auto *url{value.try_at("url")};
        if (url == nullptr || !url->is_string()) {
          return EXPECTED_DIALECT_OBJECT;
        }
      }

      for (const auto &entry : value.as_object()) {
        if (entry.first.starts_with("x-")) {
          continue;
        }

        const auto expected_string{
            keyword == "externalDocs"
                ? (entry.first == "description" || entry.first == "url")
                : (entry.first == "name" || entry.first == "namespace" ||
                   entry.first == "prefix")};
        const auto expected_boolean{
            keyword == "xml" &&
            (entry.first == "attribute" || entry.first == "wrapped")};
        if (expected_string) {
          if (!entry.second.is_string()) {
            return EXPECTED_DIALECT_OBJECT;
          }
        } else if (expected_boolean) {
          if (!entry.second.is_boolean()) {
            return EXPECTED_DIALECT_OBJECT;
          }
        } else {
          // Both of their meta-schemas close them to the members they name
          return EXPECTED_DIALECT_OBJECT;
        }
      }

      return nullptr;
    }
    if (keyword == "maxContains" || keyword == "minContains") {
      // These only exist from 2019-09 onwards, where a number whose fractional
      // part is zero counts as an integer
      return (value.is_integral() && value.is_positive())
                 ? nullptr
                 : EXPECTED_NON_NEGATIVE_INTEGER;
    }
    if (keyword == "exclusiveMaximum" || keyword == "exclusiveMinimum") {
      return ((is_draft3(vocabularies) || is_draft4(vocabularies) ||
               is_openapi_3_0(vocabularies))
                  ? (value.is_boolean() ? nullptr : EXPECTED_BOOLEAN)
                  : (value.is_number() ? nullptr : EXPECTED_NUMBER));
    }
    if ((keyword == "$defs" || keyword == "definitions") &&
        // The walker treats these as containers in every dialect, but
        // no meta-schema before Draft 4 defines either of them
        !is_draft3(vocabularies)) {
      return (value.is_object() &&
              std::ranges::all_of(
                  value.as_object(),
                  [&vocabularies, &keyword](const auto &entry) -> bool {
                    return is_keyword_schema(entry.second, vocabularies,
                                             keyword);
                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_OBJECT;
    }
  }

  // Draft 3 spells `required` as a flag on the property itself rather than as
  // a list on the object, so the shape it asks for is a different one
  if (keyword == "required" && is_draft3(vocabularies)) {
    return value.is_boolean() ? nullptr : EXPECTED_BOOLEAN;
  }

  switch (type) {
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorValueTraverseSomeProperty:
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorValueTraverseAnyPropertyKey:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseAnyItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseSomeItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseParent:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceMaybe:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceOther:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceNegate:
      return is_keyword_schema(value, vocabularies, keyword) ? nullptr
                                                             : EXPECTED_SCHEMA;
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorValueOrElementsTraverseAnyItemOrItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorValueOrElementsInPlace:
      return (is_keyword_schema(value, vocabularies, keyword) ||
              value.is_array())
                 ? nullptr
                 : EXPECTED_SCHEMA_OR_ARRAY;
    // Note that `ApplicatorElementsInPlaceSome` and its negated variant are
    // deliberately absent, as Draft 3 `type` and `disallow` also take a plain
    // type name, so their strategy does not mandate an array. So is
    // `ApplicatorMembersInPlaceSome`, as Draft 4 `dependencies` also takes a
    // list of property names as a member
    case sourcemeta::core::SchemaKeywordType::ApplicatorElementsTraverseItem:
    case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
      return (value.is_array() &&
              std::ranges::all_of(
                  value.as_array(),
                  [&vocabularies, &keyword](const auto &entry) -> bool {
                    return is_keyword_schema(entry, vocabularies, keyword);
                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_ARRAY;
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorMembersTraversePropertyStatic:
    case sourcemeta::core::SchemaKeywordType::
        ApplicatorMembersTraversePropertyRegex:
      return (value.is_object() &&
              std::ranges::all_of(
                  value.as_object(),
                  [&vocabularies, &keyword](const auto &entry) -> bool {
                    return is_keyword_schema(entry.second, vocabularies,
                                             keyword);
                  }))
                 ? nullptr
                 : EXPECTED_SCHEMA_OBJECT;
    // The walker only reports this for a keyword the dialect in use actually
    // defines, so an unknown keyword never reaches here and stays ignored, as
    // the specification requires
    case sourcemeta::core::SchemaKeywordType::Annotation:
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

// The top-level keywords of a schema in evaluation order
auto schema_keywords(const sourcemeta::core::JSON &schema,
                     const sourcemeta::core::SchemaWalker &walker,
                     const sourcemeta::core::SchemaVocabularies &vocabularies)
    -> std::vector<
        std::reference_wrapper<const sourcemeta::core::JSON::String>> {
  assert(schema.is_object());
  std::vector<std::reference_wrapper<const sourcemeta::core::JSON::String>>
      result;
  result.reserve(schema.size());
  for (const auto &entry : schema.as_object()) {
    result.emplace_back(std::cref(entry.first));
  }

  // Sort first on priority, second on the keyword itself. The latter is to
  // make sure different compilers with different STL implementations end up at
  // the exact same result
  std::ranges::sort(
      result,
      [&vocabularies, &walker](const auto left, const auto right) -> bool {
        const auto left_priority{sourcemeta::core::schema_keyword_priority(
            left.get(), vocabularies, walker)};
        const auto right_priority{sourcemeta::core::schema_keyword_priority(
            right.get(), vocabularies, walker)};
        if (left_priority != right_priority) {
          return left_priority < right_priority;
        }

        return left.get() < right.get();
      });

  return result;
}

// Compiling a subschema recurses back into itself through the keyword
// handlers, so what bounds that recursion has to survive across those calls.
// A guard that throws never incremented, and so never decrements either
class DepthGuard {
public:
  DepthGuard(std::uint64_t &depth, const std::uint64_t limit) : depth_{depth} {
    if (this->depth_ >= limit) [[unlikely]] {
      throw sourcemeta::blaze::CompilerDepthLimitError{limit};
    }

    this->depth_ += 1;
  }

  ~DepthGuard() { this->depth_ -= 1; }

  DepthGuard(const DepthGuard &) = delete;
  auto operator=(const DepthGuard &) -> DepthGuard & = delete;
  DepthGuard(DepthGuard &&) = delete;
  auto operator=(DepthGuard &&) -> DepthGuard & = delete;

private:
  std::uint64_t &depth_;
};

auto compile_subschema(const sourcemeta::blaze::KeywordContext &context)
    -> sourcemeta::blaze::Instructions {
  using namespace sourcemeta::blaze;
  assert((context.schema().is_object() || context.schema().is_boolean()));
  const DepthGuard depth_guard{context.global.depth,
                               context.global.tweaks.max_depth};

  // A boolean in a keyword position is settled by the keyword's own contract,
  // which the shape check below applies. What is left is the root of a schema
  // resource, where a dialect without boolean schemas admits no boolean at all
  if (context.schema().is_boolean() && context.local.relative_pointer.empty() &&
      !booleans_are_schemas(context.vocabularies())) [[unlikely]] {
    throw_keyword_error(context,
                        "This dialect does not support boolean schemas");
  }

  // Handle boolean schemas earlier on, as nobody should be able to
  // override what these mean.
  if (context.schema().is_boolean()) {
    if (context.schema().to_boolean()) {
      return {};
    }
    const KeywordContext failure_context{
        .global = context.global,
        .local = context.local,
        .target = {.keyword = EMPTY_KEYWORD,
                   .base_schema_location = context.target.base_schema_location,
                   .base_instance_location =
                       context.target.base_instance_location}};
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail,
                 failure_context, ValueNone{})};
  }

  const auto keywords{schema_keywords(context.schema(), context.global.walker,
                                      context.vocabularies())};

  Instructions steps;
  for (const auto keyword_reference : keywords) {
    const auto &keyword{keyword_reference.get()};
    const auto &keyword_value{context.schema().at(keyword)};
    const auto &metadata{
        context.global.walker(keyword, context.vocabularies())};
    const auto official{
        metadata.vocabulary.has_value() &&
        std::holds_alternative<sourcemeta::core::SchemaVocabularies::Known>(
            metadata.vocabulary.value())};
    const auto *shape_error{keyword_shape_error(keyword, metadata.type,
                                                official, keyword_value,
                                                context.vocabularies())};
    static const sourcemeta::core::JSON::String KEYWORD_REF{"$ref"};
    const auto overridden_by_ref{
        keyword != KEYWORD_REF && context.schema().defines(KEYWORD_REF) &&
        ref_object_ignores_siblings(context.vocabularies())};

    // Draft 3 spells these as flags on a sibling bound rather than as bounds of
    // their own, and its meta-schema asks for that sibling to be there
    static const sourcemeta::core::JSON::String KEYWORD_MINIMUM{"minimum"};
    static const sourcemeta::core::JSON::String KEYWORD_MAXIMUM{"maximum"};
    if (!overridden_by_ref && official &&
        exclusive_bounds_need_a_sibling(context.vocabularies()) &&
        ((keyword == "exclusiveMinimum" &&
          !context.schema().defines(KEYWORD_MINIMUM)) ||
         (keyword == "exclusiveMaximum" &&
          !context.schema().defines(KEYWORD_MAXIMUM)))) [[unlikely]] {
      throw sourcemeta::blaze::CompilerError(
          context.base(),
          absolute_schema_location(
              context.global, context.base(),
              context.local.relative_pointer.concat(
                  sourcemeta::blaze::make_weak_pointer(keyword))),
          "This keyword was expected to accompany the bound it applies to");
    }

    if (!overridden_by_ref && shape_error != nullptr) [[unlikely]] {
      throw sourcemeta::blaze::CompilerError(
          context.base(),
          absolute_schema_location(
              context.global, context.base(),
              context.local.relative_pointer.concat(
                  sourcemeta::blaze::make_weak_pointer(keyword))),
          shape_error);
    }

    // Bases must not contain fragments
    assert(!context.base().fragment().has_value());

    // `SchemaContext` holds this pointer by reference, and `KeywordContext`
    // holds that context the same way, so both have to outlive the loop. A
    // temporary in the range-initializer would only do so through the C++23
    // lifetime extension, which leaks it when the compiler throws from here,
    // as the depth and instruction limits do
    const auto keyword_pointer{
        context.local.relative_pointer.concat(make_weak_pointer(keyword))};
    const SchemaContext keyword_schema_context{
        .relative_pointer = keyword_pointer,
        .schema = context.schema(),
        .vocabularies = context.vocabularies(),
        .base = context.base(),
        .is_property_name = context.local.is_property_name};
    const KeywordContext keyword_context{
        .global = context.global,
        .local = keyword_schema_context,
        .target = {.keyword = keyword,
                   .keyword_value = &keyword_value,
                   .base_schema_location = context.target.base_schema_location,
                   .base_instance_location =
                       context.target.base_instance_location}};
    for (auto &&step : context.global.compiler(keyword_context, steps)) {
      // Just a sanity check to ensure every keyword location is indeed valid
      assert(sourcemeta::core::try_get(
                 context.global.root,
                 absolute_schema_pointer(keyword_context)) != nullptr);
      steps.push_back(std::move(step));
    }
  }

  // An `unevaluated*` keyword reads the marks its own subschema made, and
  // nothing else. Sibling subschemas get flattened into a single instruction
  // list, so the boundary of that scope has to be carried by an instruction
  // rather than recovered at evaluation time
  if (std::ranges::any_of(steps, [](const auto &step) -> bool {
        return step.type == InstructionIndex::LoopPropertiesUnevaluated ||
               step.type == InstructionIndex::LoopPropertiesUnevaluatedExcept ||
               step.type == InstructionIndex::LoopItemsUnevaluated;
      })) {
    const KeywordContext frame_context{
        .global = context.global,
        .local = context.local,
        .target = {.keyword = EMPTY_KEYWORD,
                   .base_schema_location = sourcemeta::core::EMPTY_WEAK_POINTER,
                   .base_instance_location =
                       sourcemeta::core::EMPTY_WEAK_POINTER}};
    return {make(InstructionIndex::ControlFrame, frame_context, ValueNone{},
                 std::move(steps))};
  }

  return steps;
}

auto defines_any_whitelisted_keyword(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::SchemaFrame::Location &entrypoint_location,
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

  return sourcemeta::core::schema_frame_any_reachable_subschema(
      frame, entrypoint_location, walker, resolver,
      [&](const sourcemeta::core::SchemaFrame::Location &location) -> bool {
        const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
        if (!subschema.is_object()) {
          return false;
        }

        for (const auto &[keyword, keyword_hash] : hashed_keywords) {
          if (subschema.defines(keyword, keyword_hash)) {
            return true;
          }
        }

        return false;
      });
}

} // namespace

namespace sourcemeta::blaze {

// Whether an entry point that the frame could not locate nonetheless names a
// place that the document has, which means it is a keyword rather than a
// schema. Only reachable when the entry point carries a pointer fragment, as
// anything else can only ever name a schema
auto entrypoint_names_non_schema(const sourcemeta::core::JSON &schema,
                                 const sourcemeta::core::SchemaFrame &frame,
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
    // A base that framing does not locate is either the one the caller framed
    // a wrapper document with, which addresses that document from its top, or
    // one that names some other document, which this one has nothing to say
    // about
    return (base.empty() ||
            frame.any_subschema(
                [&base](const sourcemeta::core::SchemaFrame::Location &location)
                    -> bool { return location.base == base; })) &&
           sourcemeta::core::try_get(schema, relative.value()) != nullptr;
  }

  return sourcemeta::core::try_get(
             schema,
             sourcemeta::core::to_pointer(base_location.value().get().pointer)
                 .concat(relative.value())) != nullptr;
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler,
             const sourcemeta::core::SchemaFrame &frame,
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
      sourcemeta::core::SchemaFrame::LocationType::Pointer) [[unlikely]] {
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
                   const sourcemeta::core::SchemaFrame::Location &) -> void {
        resources.emplace_back(uri);
      });

  // A schema that declares no identifier is still addressed by the base the
  // caller framed it with, which is what its instructions report as their
  // resource
  frame.for_each_subschema(
      [&resources](
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        if (!location.base.empty()) {
          resources.emplace_back(location.base);
        }
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

  const auto target_types{sourcemeta::core::schema_frame_target_types(frame)};

  std::map<
      std::tuple<sourcemeta::core::SchemaReferenceType, std::string_view, bool>,
      std::pair<std::size_t, const sourcemeta::core::WeakPointer *>>
      targets_map;
  targets_map.emplace(
      std::make_tuple(sourcemeta::core::SchemaReferenceType::Static, entrypoint,
                      false),
      std::make_pair(0, nullptr));

  frame.for_each_reference(
      [&](const sourcemeta::core::SchemaReferenceType type,
          const sourcemeta::core::WeakPointer &origin,
          const sourcemeta::core::SchemaFrame::Reference &reference) -> void {
        if (sourcemeta::core::schema_is_metaschema_reference(origin)) {
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
        const auto &target_type{target_types.at(reference.destination)};

        if (target_type.property_name) {
          targets_map.emplace(
              std::make_tuple(type, std::string_view{reference.destination},
                              true),
              std::make_pair(targets_map.size(), &origin));
        }

        if (target_type.instance) {
          targets_map.emplace(
              std::make_tuple(type, std::string_view{reference.destination},
                              false),
              std::make_pair(targets_map.size(), &origin));
        }
      });

  // Also add dynamic anchors that may not be directly referenced
  // but could be used as override targets during dynamic resolution
  frame.for_each_anchor(
      sourcemeta::core::SchemaReferenceType::Dynamic,
      [&](const std::string_view uri,
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        // Skip unreachable dynamic anchors
        if (!frame.is_reachable(entrypoint_location, location, walker,
                                resolver)) {
          return;
        }

        targets_map.emplace(
            std::make_tuple(sourcemeta::core::SchemaReferenceType::Dynamic, uri,
                            false),
            std::make_pair(targets_map.size(), nullptr));
      });

  ///////////////////////////////////////////////////////////////////
  // (4) Build the global compilation context
  ///////////////////////////////////////////////////////////////////

  auto unevaluated{
      sourcemeta::blaze::unevaluated(schema, frame, walker, resolver)};

  std::uint64_t compilation_depth{0};
  InstructionExtras instruction_extra{effective_tweaks.max_instructions};
  std::vector<sourcemeta::core::SchemaVocabularies::URI>
      instruction_vocabularies;
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
                        .depth = compilation_depth,
                        .extra = instruction_extra,
                        .vocabularies = instruction_vocabularies};

  ///////////////////////////////////////////////////////////////////
  // (5) Build labels map for dynamic anchors
  ///////////////////////////////////////////////////////////////////

  std::vector<std::pair<std::size_t, std::size_t>> labels_map;
  if (uses_dynamic_scopes) {
    context.frame.for_each_anchor(
        sourcemeta::core::SchemaReferenceType::Dynamic,
        [&](const std::string_view uri,
            const sourcemeta::core::SchemaFrame::Location &entry) -> void {
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
              std::make_tuple(sourcemeta::core::SchemaReferenceType::Dynamic,
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

    if (entry.type != sourcemeta::core::SchemaFrame::LocationType::Subschema &&
        entry.type != sourcemeta::core::SchemaFrame::LocationType::Resource &&
        entry.type != sourcemeta::core::SchemaFrame::LocationType::Anchor)
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

    const KeywordContext target_context{
        .global = context,
        .local = schema_context,
        .target = {.keyword = EMPTY_KEYWORD,
                   .base_schema_location = sourcemeta::core::EMPTY_WEAK_POINTER,
                   .base_instance_location =
                       sourcemeta::core::EMPTY_WEAK_POINTER}};

    compiled_targets[index] =
        compile(target_context, sourcemeta::core::EMPTY_WEAK_POINTER,
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

  // The template carries only the message templates this schema refers to
  std::vector<std::string> template_messages;
  assign_messages(compiled_targets, instruction_extra, template_messages);

  // The template stores vocabularies as plain strings, as the evaluator does
  // not otherwise depend on the schema machinery
  std::vector<std::string> template_vocabularies;
  template_vocabularies.reserve(instruction_vocabularies.size());
  for (const auto &vocabulary : instruction_vocabularies) {
    template_vocabularies.push_back(std::format("{}", vocabulary));
  }

  const bool frames{
      requires_evaluation(context, entrypoint_location.pointer) ||
      // TODO: This expression should go away if we start properly compiling
      // `unevaluatedItems` like we compile `unevaluatedProperties`
      std::ranges::any_of(
          context.unevaluated, [](const auto &dependency) -> auto {
            return dependency.first.ends_with("unevaluatedItems");
          })};
  return {.dynamic = uses_dynamic_scopes,
          .frames = frames,
          .targets = std::move(compiled_targets),
          .labels = std::move(labels_map),
          .extra = std::move(instruction_extra).release(),
          .vocabularies = std::move(template_vocabularies),
          .messages = std::move(template_messages)};
}

auto compile(const sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const Compiler &compiler, const Mode mode,
             const std::string_view default_dialect,
             const std::string_view default_id,
             const std::string_view entrypoint,
             const std::optional<Tweaks> &tweaks,
             const std::uint64_t max_locations) -> Template {
  assert((schema.is_object() || schema.is_boolean()));

  // Make sure the input schema is bundled, otherwise we won't be able to
  // resolve remote references here. Meta-schemas are not needed, as we
  // can determine vocabularies through the resolver
  sourcemeta::core::SchemaBundleOptions bundle_options;
  bundle_options.mode = sourcemeta::core::SchemaBundleOptions::Mode::References;
  bundle_options.max_locations = max_locations;
  const sourcemeta::core::JSON result{sourcemeta::core::schema_bundle(
      schema, walker, resolver, default_dialect, default_id, bundle_options)};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References,
      result,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::core::SchemaFrame::IdentifierMode::Additional,
      {sourcemeta::core::EMPTY_WEAK_POINTER},
      "",
      max_locations};
  return compile(result, walker, resolver, compiler, frame,
                 entrypoint.empty() ? frame.root() : entrypoint, mode, tweaks);
}

auto compile(const sourcemeta::blaze::KeywordContext &context,
             const sourcemeta::core::WeakPointer &schema_suffix,
             const sourcemeta::core::WeakPointer &instance_suffix,
             const std::optional<std::string_view> uri) -> Instructions {
  // An explicit URI is a jump elsewhere in the schema, which only the frame
  // can resolve. Without one we are recursing within the subschema we are
  // already at, so where we land follows from the pointer we came in with
  std::optional<
      std::reference_wrapper<const sourcemeta::core::SchemaFrame::Location>>
      entry;
  sourcemeta::core::WeakPointer target;
  if (uri.has_value()) {
    const auto destination{sourcemeta::core::URI::canonicalize(uri.value())};
    entry = context.global.frame.location(
        sourcemeta::core::SchemaReferenceType::Static, destination);
    // Otherwise the recursion attempt is non-sense
    if (!entry.has_value()) [[unlikely]] {
      throw sourcemeta::core::SchemaReferenceError(
          "The target of the reference does not exist in the schema",
          destination, absolute_schema_location(context));
    }

    target = entry.value().get().pointer;
  } else {
    target = absolute_schema_pointer(context).concat(schema_suffix);
    // Otherwise the recursion attempt is non-sense
    if (sourcemeta::core::try_get(context.global.root, target) == nullptr)
        [[unlikely]] {
      throw sourcemeta::core::SchemaReferenceError(
          "The target of the reference does not exist in the schema",
          to_uri(context.local.relative_pointer.concat(schema_suffix),
                 context.base())
              .canonicalize()
              .recompose(),
          absolute_schema_location(context));
    }

    // A pointer that sits under more than one base is framed once per base, so
    // this may be any of those entries. They are interchangeable here, as
    // framing resolves the base, the dialect and the depth of a location from
    // the resource nearest to it rather than from the URI it got keyed under,
    // so every entry of a given pointer reports the same ones
    entry = context.global.frame.traverse(target);
  }

  const auto &new_schema{get(context.global.root, target)};

  // An invalid schema may set an applicator to a value that is not a schema
  // at all, in which case we consider it to impose no constraints
  if (!new_schema.is_object() && !new_schema.is_boolean()) [[unlikely]] {
    return {};
  }

  const sourcemeta::core::WeakPointer destination_pointer{
      context.keyword().empty()
          ? context.target.base_schema_location.concat(schema_suffix)
          : context.target.base_schema_location
                .concat(make_weak_pointer(context.keyword()))
                .concat(schema_suffix)};

  // A schema that the walker never descended into has no location of its own,
  // in which case it inherits the resource and the dialect of what encloses it
  const auto new_relative_pointer{
      entry.has_value() ? entry.value().get().pointer.slice(
                              entry.value().get().relative_pointer)
                        : context.local.relative_pointer.concat(schema_suffix)};
  const sourcemeta::core::URI new_base{
      entry.has_value()
          ? sourcemeta::core::URI{entry.value().get().base}
                .recompose_without_fragment()
                .value_or("")
          : context.base().recompose_without_fragment().value_or("")};
  const auto new_vocabularies{
      entry.has_value() ? context.global.frame.vocabularies(
                              entry.value().get(), context.global.resolver)
                        : context.vocabularies()};

  // The root check on dialects without boolean schemas cannot see a boolean
  // reference target, as a target compiles with the pointer of wherever it
  // sits rather than as the root of a resource. Only a reference lands here
  // with a URI: a boolean a keyword admits in place of a schema comes in
  // through the suffixes instead, so it stays out of this one's way
  if (uri.has_value() && new_schema.is_boolean() &&
      new_vocabularies.contains(
          sourcemeta::core::SchemaVocabularies::Known::OPENAPI_3_0_BASE))
      [[unlikely]] {
    throw sourcemeta::blaze::CompilerError(
        new_base, sourcemeta::core::to_pointer(target),
        "This dialect does not support boolean schemas");
  }

  const SchemaContext new_schema_context{
      .relative_pointer = new_relative_pointer,
      .schema = new_schema,
      .vocabularies = new_vocabularies,
      .base = new_base,
      .is_property_name = context.local.is_property_name};
  // The keyword travels on so that the instructions the subschema makes stay
  // addressed from it, but the schema it names is no longer the one at hand,
  // so the value it is set to does not travel with it
  return compile_subschema(
      {.global = context.global,
       .local = new_schema_context,
       .target = {
           .keyword = context.keyword(),
           .base_schema_location = destination_pointer,
           .base_instance_location =
               context.target.base_instance_location.concat(instance_suffix)}});
}

} // namespace sourcemeta::blaze
