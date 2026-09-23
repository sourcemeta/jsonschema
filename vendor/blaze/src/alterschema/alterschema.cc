#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/uri.h>

#include "schema_helpers.h"

// For built-in rules
#include <algorithm>     // std::sort, std::unique, std::ranges::none_of
#include <array>         // std::array
#include <bit>           // std::popcount
#include <cassert>       // assert
#include <cmath>         // std::floor, std::ceil, std::isfinite
#include <cstddef>       // std::size_t
#include <functional>    // std::ref, std::reference_wrapper
#include <iterator>      // std::back_inserter
#include <limits>        // std::numeric_limits
#include <memory>        // std::unique_ptr, std::make_unique
#include <optional>      // std::optional
#include <sstream>       // std::ostringstream
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::to_underlying

namespace sourcemeta::blaze {

using namespace sourcemeta::core;

template <typename... Args>
auto applies_to_keywords(Args &&...args) -> SchemaTransformRule::Result {
  std::vector<Pointer> result;
  result.reserve(sizeof...(args));
  (result.push_back(Pointer{std::forward<Args>(args)}), ...);
  return result;
}

inline auto applies_to_pointers(std::vector<Pointer> &&keywords)
    -> SchemaTransformRule::Result {
  return {std::move(keywords)};
}

// TODO: Move upstream
inline auto is_in_place_applicator(const SchemaKeywordType type) -> bool {
  return type == SchemaKeywordType::ApplicatorValueOrElementsInPlace ||
         type == SchemaKeywordType::ApplicatorMembersInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlace ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSomeNegate ||
         type == SchemaKeywordType::ApplicatorValueInPlaceMaybe ||
         type == SchemaKeywordType::ApplicatorValueInPlaceOther ||
         type == SchemaKeywordType::ApplicatorValueInPlaceNegate;
}

// Whether a `type` value only consists of simple type names that can be
// parsed into a complete set of instance types. Draft 0 to Draft 3 unions
// may contain subschemas or `any`, in which case the parsed set is an
// under-approximation that cannot be trusted. Later dialects do not give
// such forms any meaning, so the parsed set stands
inline auto is_known_type_form(const JSON &type,
                               const SchemaVocabularies &vocabularies) -> bool {
  if (!vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
    return true;
  }
  if (type.is_string()) {
    return type.to_string() != "any";
  }
  if (!type.is_array()) {
    return false;
  }
  return std::ranges::all_of(type.as_array(), [](const auto &entry) -> auto {
    return entry.is_string() && entry.to_string() != "any";
  });
}

// Walk up from a schema location, continuing as long as the traversal
// predicate returns true for each keyword type encountered. Returns a
// reference to the pointer of the ancestor where the match callback returned
// true, or nullopt if no match was found or the traversal predicate stopped
// the walk.
template <typename TraversePredicate, typename MatchCallback>
auto walk_up(const JSON &root, const SchemaFrame &frame,
             const SchemaFrame::Location &location, const SchemaWalker &walker,
             const SchemaResolver &resolver,
             const TraversePredicate &should_continue,
             const MatchCallback &matches)
    -> std::optional<std::reference_wrapper<const WeakPointer>> {
  auto current_pointer{location.pointer};
  auto current_parent{location.parent};

  while (current_parent.has_value()) {
    const auto &parent_pointer{current_parent.value()};
    const auto relative_pointer{current_pointer.resolve_from(parent_pointer)};
    assert(!relative_pointer.empty() && relative_pointer.at(0).is_property());
    const auto parent{frame.traverse(parent_pointer)};
    assert(parent.has_value());
    const auto &parent_vocabularies{
        frame.vocabularies(parent.value().get(), resolver)};
    const auto keyword_type{
        walker(relative_pointer.at(0).to_property(), parent_vocabularies).type};

    if (!should_continue(keyword_type)) {
      return std::nullopt;
    }

    if (matches(get(root, parent_pointer), parent_vocabularies)) {
      return std::cref(parent.value().get().pointer);
    }

    current_pointer = parent_pointer;
    current_parent = parent.value().get().parent;
  }

  return std::nullopt;
}

template <typename MatchCallback>
auto walk_up_in_place_applicators(const JSON &root, const SchemaFrame &frame,
                                  const SchemaFrame::Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver,
                                  const MatchCallback &matches)
    -> std::optional<std::reference_wrapper<const WeakPointer>> {
  return walk_up(root, frame, location, walker, resolver,
                 is_in_place_applicator, matches);
}

// Compile a subschema of a wrapper document, whose frame locates schemas
// within it rather than one at its top. Every schema that the frame locates is
// bundled into a copy of the document at once, so that the subschema may
// reference any of them as well as a remote schema
inline auto compile_embedded_subschema(const JSON &root,
                                       const SchemaFrame &frame,
                                       const SchemaFrame::Location &location,
                                       const SchemaWalker &walker,
                                       const SchemaResolver &resolver,
                                       const Compiler &compiler) -> Template {
  std::unordered_set<Pointer, Pointer::Hasher> roots;
  std::string_view default_dialect{location.dialect};
  std::string_view default_base;
  frame.for_each_subschema([&](const SchemaFrame::Location &entry) -> void {
    if (entry.parent.has_value()) {
      return;
    }

    const auto [iterator, inserted] = roots.insert(to_pointer(entry.pointer));
    if (!inserted) {
      return;
    }

    // A schema that declares no dialect of its own reports the default one
    // that the document was framed with
    if (declared_dialect(get(root, *iterator)).empty()) {
      default_dialect = entry.dialect;
    }

    // A schema that declares no identifier of its own is addressed from the
    // top of the document, by the base that the document was framed with
    if (entry.relative_pointer == 0) {
      default_base = entry.base;
    }
  });

  SchemaFrame::Paths paths;
  paths.reserve(roots.size());
  for (const auto &pointer : roots) {
    paths.push_back(to_weak_pointer(pointer));
  }

  // A wrapper document may declare a member of this name of its own, so bundle
  // into one that it does not declare, which then only holds what bundling
  // embedded
  JSON::String container_name{"x-sourcemeta-embedded"};
  while (root.defines(container_name)) {
    container_name.push_back('_');
  }

  const Pointer container{container_name};
  auto document{root};
  SchemaBundleOptions options;
  options.mode = SchemaBundleOptions::Mode::References;
  options.default_container = container;
  options.paths = paths;
  options.default_base = default_base;
  schema_bundle(document, walker, resolver, default_dialect, "", options);

  // Bundling embeds every remote schema into the container, outside of every
  // schema that the frame located, so each of them is framed as a schema too
  std::vector<Pointer> embedded;
  const auto *remotes{document.try_at(container_name)};
  if (remotes != nullptr) {
    embedded.reserve(remotes->size());
    for (const auto &entry : remotes->as_object()) {
      embedded.push_back(container.concat(Pointer{entry.first}));
    }
  }

  for (const auto &pointer : embedded) {
    paths.push_back(to_weak_pointer(pointer));
  }

  const SchemaFrame bundled_frame{SchemaFrame::Mode::References,
                                  document,
                                  walker,
                                  resolver,
                                  default_dialect,
                                  "",
                                  SchemaFrame::IdentifierMode::Additional,
                                  paths,
                                  default_base};
  const auto entrypoint{bundled_frame.uri(location.pointer)};
  assert(entrypoint.has_value());
  return compile(document, walker, resolver, compiler, bundled_frame,
                 entrypoint.value().get(), Mode::Exhaustive);
}

// Compile the subschema at a location of a frame that does not stand alone,
// setting the base that evaluate paths are relative to, or report nothing if
// it does not compile
inline auto compile_non_standalone_subschema(
    const JSON &root, const SchemaFrame &frame,
    const SchemaFrame::Location &location, const SchemaWalker &walker,
    const SchemaResolver &resolver, const Compiler &compiler, WeakPointer &base)
    -> std::optional<Template> {
  // A frame that locates schemas within a wrapper document has no schema at
  // the top of that document to wrap
  const auto embedded{!frame.traverse(EMPTY_WEAK_POINTER).has_value()};
  std::optional<JSON> subschema;
  std::string_view default_id;
  if (!embedded) {
    // Deliberately framed without a default identifier, so that the root
    // comes back empty exactly when the schema declares none of its own
    const SchemaFrame declared_frame{SchemaFrame::Mode::Root, root, walker,
                                     resolver, location.dialect};
    default_id = location.base;
    if (!declared_frame.root().empty() || default_id.empty()) {
      default_id = "";
    }

    subschema.emplace(wrap(root, frame, location, walker, resolver, base));
  }

  try {
    return embedded ? compile_embedded_subschema(root, frame, location, walker,
                                                 resolver, compiler)
                    : compile(subschema.value(), walker, resolver, compiler,
                              Mode::Exhaustive, location.dialect, default_id);
  } catch (const CompilerReferenceTargetNotSchemaError &) {
    throw;
  } catch (const SchemaVocabularyError &) {
    throw;
  } catch (...) {
    return std::nullopt;
  }
}

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

// Common
#include "common/allof_false_simplify.h"
#include "common/anyof_false_simplify.h"
#include "common/anyof_remove_false_schemas.h"
#include "common/anyof_true_simplify.h"
#include "common/const_in_enum.h"
#include "common/const_with_type.h"
#include "common/content_media_type_without_encoding.h"
#include "common/content_schema_without_media_type.h"
#include "common/dependencies_property_tautology.h"
#include "common/dependent_required_tautology.h"
#include "common/disallow_narrows_type.h"
#include "common/double_negation_elimination.h"
#include "common/draft_official_dialect_with_https.h"
#include "common/draft_official_dialect_without_empty_fragment.h"
#include "common/draft_ref_siblings.h"
#include "common/drop_allof_empty_schemas.h"
#include "common/drop_extends_empty_schemas.h"
#include "common/duplicate_allof_branches.h"
#include "common/duplicate_anyof_branches.h"
#include "common/duplicate_enum_values.h"
#include "common/duplicate_required_values.h"
#include "common/dynamic_ref_to_static_ref.h"
#include "common/else_without_if.h"
#include "common/empty_object_as_true.h"
#include "common/enum_with_type.h"
#include "common/equal_numeric_bounds_to_enum.h"
#include "common/exclusive_bounds_false_drop.h"
#include "common/exclusive_maximum_number_and_maximum.h"
#include "common/exclusive_minimum_number_and_minimum.h"
#include "common/flatten_nested_allof.h"
#include "common/flatten_nested_anyof.h"
#include "common/flatten_nested_extends.h"
#include "common/if_without_then_else.h"
#include "common/ignored_metaschema.h"
#include "common/max_contains_without_contains.h"
#include "common/maximum_real_for_integer.h"
#include "common/min_contains_without_contains.h"
#include "common/minimum_real_for_integer.h"
#include "common/modern_official_dialect_with_empty_fragment.h"
#include "common/modern_official_dialect_with_http.h"
#include "common/non_applicable_additional_items.h"
#include "common/non_applicable_disallow_types.h"
#include "common/non_applicable_enum_validation_keywords.h"
#include "common/non_applicable_type_specific_keywords.h"
#include "common/not_false.h"
#include "common/oneof_false_simplify.h"
#include "common/oneof_to_anyof_disjoint_types.h"
#include "common/orphan_definitions.h"
#include "common/required_properties_in_properties.h"
#include "common/single_type_array.h"
#include "common/then_without_if.h"
#include "common/unknown_keywords_prefix.h"
#include "common/unknown_local_ref.h"
#include "common/unnecessary_allof_ref_wrapper_draft.h"
#include "common/unnecessary_extends_ref_wrapper.h"
#include "common/unsatisfiable_drop_validation.h"
#include "common/unsatisfiable_in_place_applicator_type.h"
#include "linter/else_empty.h"
#include "linter/then_empty.h"
#include "linter/unnecessary_allof_ref_wrapper_modern.h"
#include "linter/unnecessary_allof_wrapper.h"
#include "linter/unnecessary_extends_wrapper.h"

// Linter
#include "linter/comment_trim.h"
#include "linter/conflicting_readonly_writeonly.h"
#include "linter/const_not_in_enum.h"
#include "linter/content_schema_default.h"
#include "linter/definitions_to_defs.h"
#include "linter/dependencies_default.h"
#include "linter/dependent_required_default.h"
#include "linter/description_trailing_period.h"
#include "linter/description_trim.h"
#include "linter/disallow_default.h"
#include "linter/divisible_by_default.h"
#include "linter/duplicate_examples.h"
#include "linter/enum_to_const.h"
#include "linter/equal_numeric_bounds_to_const.h"
#include "linter/forbid_empty_enum.h"
#include "linter/incoherent_min_max_contains.h"
#include "linter/invalid_external_ref.h"
#include "linter/items_array_default.h"
#include "linter/items_schema_default.h"
#include "linter/multiple_of_default.h"
#include "linter/pattern_non_ecma_regex.h"
#include "linter/pattern_properties_default.h"
#include "linter/pattern_properties_non_ecma_regex.h"
#include "linter/portable_anchor_names.h"
#include "linter/properties_default.h"
#include "linter/property_names_default.h"
#include "linter/property_names_type_default.h"
#include "linter/simple_properties_identifiers.h"
#include "linter/title_description_equal.h"
#include "linter/title_trailing_period.h"
#include "linter/title_trim.h"
#include "linter/top_level_description.h"
#include "linter/top_level_examples.h"
#include "linter/top_level_title.h"
#include "linter/unevaluated_items_default.h"
#include "linter/unevaluated_properties_default.h"
#include "linter/unknown_format_prefix.h"
#include "linter/unsatisfiable_max_contains.h"
#include "linter/unsatisfiable_min_properties.h"
#include "linter/valid_default.h"
#include "linter/valid_examples.h"

#undef ONLY_CONTINUE_IF
} // namespace sourcemeta::blaze

namespace sourcemeta::blaze {

auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void {
  if (mode == AlterSchemaMode::Linter) {
    bundle.add<DefinitionsToDefs>();
  }

  bundle.add<ContentMediaTypeWithoutEncoding>();
  bundle.add<ContentSchemaWithoutMediaType>();
  bundle.add<DraftOfficialDialectWithHttps>();
  bundle.add<DraftOfficialDialectWithoutEmptyFragment>();
  bundle.add<NonApplicableTypeSpecificKeywords>();
  bundle.add<NonApplicableDisallowTypes>();
  bundle.add<DisallowNarrowsType>();
  bundle.add<AnyOfRemoveFalseSchemas>();
  bundle.add<AnyOfTrueSimplify>();
  bundle.add<DuplicateAllOfBranches>();
  bundle.add<DuplicateAnyOfBranches>();
  bundle.add<FlattenNestedAllOf>();
  bundle.add<FlattenNestedExtends>();
  bundle.add<FlattenNestedAnyOf>();
  bundle.add<UnsatisfiableInPlaceApplicatorType>();
  bundle.add<AllOfFalseSimplify>();
  bundle.add<AnyOfFalseSimplify>();
  bundle.add<OneOfFalseSimplify>();
  bundle.add<DoubleNegationElimination>();
  bundle.add<OneOfToAnyOfDisjointTypes>();
  bundle.add<UnsatisfiableDropValidation>();
  bundle.add<ElseWithoutIf>();
  bundle.add<IfWithoutThenElse>();
  bundle.add<IgnoredMetaschema>();
  bundle.add<MaxContainsWithoutContains>();
  bundle.add<MinContainsWithoutContains>();
  bundle.add<NotFalse>();
  bundle.add<ThenEmpty>();
  bundle.add<ElseEmpty>();
  bundle.add<ThenWithoutIf>();
  bundle.add<DependenciesPropertyTautology>();
  bundle.add<DependentRequiredTautology>();
  bundle.add<EqualNumericBoundsToEnum>();
  bundle.add<MaximumRealForInteger>();
  bundle.add<MinimumRealForInteger>();
  bundle.add<SingleTypeArray>();
  bundle.add<EnumWithType>();
  bundle.add<NonApplicableEnumValidationKeywords>();
  bundle.add<DuplicateEnumValues>();
  bundle.add<DuplicateRequiredValues>();
  bundle.add<ConstWithType>();
  bundle.add<ConstInEnum>();
  bundle.add<NonApplicableAdditionalItems>();
  bundle.add<ModernOfficialDialectWithEmptyFragment>();
  bundle.add<ModernOfficialDialectWithHttp>();
  bundle.add<ExclusiveMaximumNumberAndMaximum>();
  bundle.add<ExclusiveMinimumNumberAndMinimum>();
  bundle.add<ExclusiveBoundsFalseDrop>();
  bundle.add<DraftRefSiblings>();
  bundle.add<DynamicRefToStaticRef>();
  bundle.add<UnknownKeywordsPrefix>();
  bundle.add<UnknownLocalRef>();
  bundle.add<RequiredPropertiesInProperties>();
  bundle.add<OrphanDefinitions>();

  if (mode == AlterSchemaMode::Linter) {
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<ConstNotInEnum>();
    bundle.add<ContentSchemaDefault>();
    bundle.add<DependenciesDefault>();
    bundle.add<DependentRequiredDefault>();
    bundle.add<ItemsArrayDefault>();
    bundle.add<ItemsSchemaDefault>();
    bundle.add<DisallowDefault>();
    bundle.add<DivisibleByDefault>();
    bundle.add<MultipleOfDefault>();
    bundle.add<PatternPropertiesDefault>();
    bundle.add<PatternNonEcmaRegex>();
    bundle.add<PatternPropertiesNonEcmaRegex>();
    bundle.add<PropertiesDefault>();
    bundle.add<PropertyNamesDefault>();
    bundle.add<PropertyNamesTypeDefault>();
    bundle.add<UnevaluatedItemsDefault>();
    bundle.add<UnevaluatedPropertiesDefault>();
    bundle.add<UnsatisfiableMaxContains>();
    bundle.add<IncoherentMinMaxContains>();
    bundle.add<UnsatisfiableMinProperties>();
    bundle.add<EnumToConst>();
    bundle.add<ForbidEmptyEnum>();
    bundle.add<TopLevelTitle>();
    bundle.add<TopLevelDescription>();
    bundle.add<TopLevelExamples>();
    bundle.add<TitleDescriptionEqual>();
    bundle.add<TitleTrailingPeriod>();
    bundle.add<DescriptionTrailingPeriod>();
    bundle.add<TitleTrim>();
    bundle.add<DescriptionTrim>();
    bundle.add<CommentTrim>();
    bundle.add<ConflictingReadOnlyWriteOnly>();
    bundle.add<DuplicateExamples>();
    bundle.add<SimplePropertiesIdentifiers>();
    bundle.add<PortableAnchorNames>();
    bundle.add<InvalidExternalRef>();
    bundle.add<UnknownFormatPrefix>();
    bundle.add<ValidDefault>();
    bundle.add<ValidExamples>();
  }

  bundle.add<UnnecessaryAllOfRefWrapperModern>();
  bundle.add<UnnecessaryAllOfRefWrapperDraft>();
  bundle.add<UnnecessaryExtendsRefWrapper>();
  bundle.add<UnnecessaryAllOfWrapper>();
  bundle.add<UnnecessaryExtendsWrapper>();

  bundle.add<DropAllOfEmptySchemas>();
  bundle.add<DropExtendsEmptySchemas>();
  bundle.add<EmptyObjectAsTrue>();
}

} // namespace sourcemeta::blaze
