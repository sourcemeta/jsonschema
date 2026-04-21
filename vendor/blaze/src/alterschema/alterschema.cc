#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/core/regex.h>

// For built-in rules
#include <algorithm>     // std::sort, std::unique, std::ranges::none_of
#include <array>         // std::array
#include <bit>           // std::popcount
#include <cassert>       // assert
#include <cmath>         // std::floor, std::ceil, std::isfinite
#include <cstddef>       // std::size_t
#include <functional>    // std::ref
#include <iterator>      // std::back_inserter
#include <limits>        // std::numeric_limits
#include <memory>        // std::unique_ptr, std::make_unique
#include <sstream>       // std::ostringstream
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::to_underlying

namespace sourcemeta::blaze {

using namespace sourcemeta::core;

template <typename... Args>
auto APPLIES_TO_KEYWORDS(Args &&...args) -> SchemaTransformRule::Result {
  std::vector<Pointer> result;
  result.reserve(sizeof...(args));
  (result.push_back(Pointer{std::forward<Args>(args)}), ...);
  return result;
}

inline auto APPLIES_TO_POINTERS(std::vector<Pointer> &&keywords)
    -> SchemaTransformRule::Result {
  return {std::move(keywords)};
}

// TODO: Move upstream
inline auto IS_IN_PLACE_APPLICATOR(const SchemaKeywordType type) -> bool {
  return type == SchemaKeywordType::ApplicatorValueOrElementsInPlace ||
         type == SchemaKeywordType::ApplicatorMembersInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlace ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSomeNegate ||
         type == SchemaKeywordType::ApplicatorValueInPlaceMaybe ||
         type == SchemaKeywordType::ApplicatorValueInPlaceOther ||
         type == SchemaKeywordType::ApplicatorValueInPlaceNegate;
}

// Walk up from a schema location, continuing as long as the traversal
// predicate returns true for each keyword type encountered. Returns a
// reference to the pointer of the ancestor where the match callback returned
// true, or nullopt if no match was found or the traversal predicate stopped
// the walk.
template <typename TraversePredicate, typename MatchCallback>
auto WALK_UP(const JSON &root, const SchemaFrame &frame,
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
    const auto parent{frame.traverse(frame.uri(parent_pointer).value().get())};
    assert(parent.has_value());
    const auto parent_vocabularies{
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
auto WALK_UP_IN_PLACE_APPLICATORS(const JSON &root, const SchemaFrame &frame,
                                  const SchemaFrame::Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver,
                                  const MatchCallback &matches)
    -> std::optional<std::reference_wrapper<const WeakPointer>> {
  return WALK_UP(root, frame, location, walker, resolver,
                 IS_IN_PLACE_APPLICATOR, matches);
}

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

#include "canonicalizer/additional_items_implicit.h"
#include "canonicalizer/comment_drop.h"
#include "canonicalizer/const_as_enum.h"
#include "canonicalizer/dependencies_to_any_of.h"
#include "canonicalizer/dependencies_to_extends_disallow.h"
#include "canonicalizer/dependent_required_to_any_of.h"
#include "canonicalizer/dependent_schemas_to_any_of.h"
#include "canonicalizer/deprecated_false_drop.h"
#include "canonicalizer/disallow_to_array_of_schemas.h"
#include "canonicalizer/divisible_by_implicit.h"
#include "canonicalizer/empty_definitions_drop.h"
#include "canonicalizer/empty_defs_drop.h"
#include "canonicalizer/empty_dependencies_drop.h"
#include "canonicalizer/empty_dependent_required_drop.h"
#include "canonicalizer/empty_dependent_schemas_drop.h"
#include "canonicalizer/enum_drop_redundant_validation.h"
#include "canonicalizer/enum_filter_by_type.h"
#include "canonicalizer/exclusive_bounds_false_drop.h"
#include "canonicalizer/exclusive_maximum_boolean_integer_fold.h"
#include "canonicalizer/exclusive_maximum_integer_to_maximum.h"
#include "canonicalizer/exclusive_minimum_boolean_integer_fold.h"
#include "canonicalizer/exclusive_minimum_integer_to_minimum.h"
#include "canonicalizer/extends_to_array.h"
#include "canonicalizer/if_then_else_implicit.h"
#include "canonicalizer/implicit_contains_keywords.h"
#include "canonicalizer/implicit_object_keywords.h"
#include "canonicalizer/items_implicit.h"
#include "canonicalizer/max_contains_covered_by_max_items.h"
#include "canonicalizer/max_decimal_implicit.h"
#include "canonicalizer/maximum_can_equal_integer_fold.h"
#include "canonicalizer/maximum_can_equal_true_drop.h"
#include "canonicalizer/min_items_given_min_contains.h"
#include "canonicalizer/min_length_implicit.h"
#include "canonicalizer/min_properties_covered_by_required.h"
#include "canonicalizer/minimum_can_equal_integer_fold.h"
#include "canonicalizer/minimum_can_equal_true_drop.h"
#include "canonicalizer/multiple_of_implicit.h"
#include "canonicalizer/optional_property_implicit.h"
#include "canonicalizer/recursive_anchor_false_drop.h"
#include "canonicalizer/required_property_implicit.h"
#include "canonicalizer/single_branch_allof.h"
#include "canonicalizer/single_branch_anyof.h"
#include "canonicalizer/single_branch_oneof.h"
#include "canonicalizer/type_array_to_any_of.h"
#include "canonicalizer/type_boolean_as_enum.h"
#include "canonicalizer/type_inherit_in_place.h"
#include "canonicalizer/type_null_as_enum.h"
#include "canonicalizer/type_union_implicit.h"
#include "canonicalizer/type_union_to_schemas.h"
#include "canonicalizer/type_with_applicator_to_allof.h"
#include "canonicalizer/type_with_applicator_to_extends.h"
#include "canonicalizer/unevaluated_items_to_items.h"
#include "canonicalizer/unevaluated_properties_to_additional_properties.h"
#include "canonicalizer/unsatisfiable_can_equal_bounds.h"
#include "canonicalizer/unsatisfiable_exclusive_equal_bounds.h"
#include "canonicalizer/unsatisfiable_type_and_enum.h"

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
#include "common/double_negation_elimination.h"
#include "common/draft_official_dialect_with_https.h"
#include "common/draft_official_dialect_without_empty_fragment.h"
#include "common/draft_ref_siblings.h"
#include "common/drop_allof_empty_schemas.h"
#include "common/duplicate_allof_branches.h"
#include "common/duplicate_anyof_branches.h"
#include "common/duplicate_enum_values.h"
#include "common/duplicate_required_values.h"
#include "common/dynamic_ref_to_static_ref.h"
#include "common/else_without_if.h"
#include "common/empty_object_as_true.h"
#include "common/enum_with_type.h"
#include "common/equal_numeric_bounds_to_enum.h"
#include "common/exclusive_maximum_number_and_maximum.h"
#include "common/exclusive_minimum_number_and_minimum.h"
#include "common/flatten_nested_allof.h"
#include "common/flatten_nested_anyof.h"
#include "common/if_without_then_else.h"
#include "common/ignored_metaschema.h"
#include "common/max_contains_without_contains.h"
#include "common/maximum_real_for_integer.h"
#include "common/min_contains_without_contains.h"
#include "common/minimum_real_for_integer.h"
#include "common/modern_official_dialect_with_empty_fragment.h"
#include "common/modern_official_dialect_with_http.h"
#include "common/non_applicable_additional_items.h"
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
#include "common/unsatisfiable_drop_validation.h"
#include "common/unsatisfiable_in_place_applicator_type.h"
#include "linter/else_empty.h"
#include "linter/then_empty.h"
#include "linter/unnecessary_allof_ref_wrapper_modern.h"
#include "linter/unnecessary_allof_wrapper.h"

// Linter
#include "linter/comment_trim.h"
#include "linter/const_not_in_enum.h"
#include "linter/content_schema_default.h"
#include "linter/definitions_to_defs.h"
#include "linter/dependencies_default.h"
#include "linter/dependent_required_default.h"
#include "linter/description_trailing_period.h"
#include "linter/description_trim.h"
#include "linter/duplicate_examples.h"
#include "linter/enum_to_const.h"
#include "linter/equal_numeric_bounds_to_const.h"
#include "linter/forbid_empty_enum.h"
#include "linter/incoherent_min_max_contains.h"
#include "linter/invalid_external_ref.h"
#include "linter/items_array_default.h"
#include "linter/items_schema_default.h"
#include "linter/multiple_of_default.h"
#include "linter/pattern_properties_default.h"
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
#include "linter/unsatisfiable_max_contains.h"
#include "linter/unsatisfiable_min_properties.h"
#include "linter/valid_default.h"
#include "linter/valid_examples.h"

#undef ONLY_CONTINUE_IF
} // namespace sourcemeta::blaze

namespace sourcemeta::blaze {

auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void {
  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<ExclusiveMinimumBooleanIntegerFold>();
    bundle.add<ExclusiveMaximumBooleanIntegerFold>();
    bundle.add<ExclusiveBoundsFalseDrop>();
    bundle.add<UnsatisfiableExclusiveEqualBounds>();
    bundle.add<MinimumCanEqualIntegerFold>();
    bundle.add<MaximumCanEqualIntegerFold>();
    bundle.add<MinimumCanEqualTrueDrop>();
    bundle.add<MaximumCanEqualTrueDrop>();
    bundle.add<CommentDrop>();
    bundle.add<DeprecatedFalseDrop>();
    bundle.add<RecursiveAnchorFalseDrop>();
    bundle.add<UnevaluatedItemsToItems>();
    bundle.add<UnevaluatedPropertiesToAdditionalProperties>();
    bundle.add<IfThenElseImplicit>();
    bundle.add<ImplicitObjectKeywords>();
    bundle.add<ImplicitContainsKeywords>();
    bundle.add<ExtendsToArray>();
    bundle.add<DisallowToArrayOfSchemas>();
  }

  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<TypeInheritInPlace>();
    bundle.add<TypeUnionImplicit>();
    bundle.add<TypeArrayToAnyOf>();
  }

  if (mode == AlterSchemaMode::Linter ||
      mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<DefinitionsToDefs>();
  }

  bundle.add<ContentMediaTypeWithoutEncoding>();
  bundle.add<ContentSchemaWithoutMediaType>();
  bundle.add<DraftOfficialDialectWithHttps>();
  bundle.add<DraftOfficialDialectWithoutEmptyFragment>();
  bundle.add<NonApplicableTypeSpecificKeywords>();
  bundle.add<AnyOfRemoveFalseSchemas>();
  bundle.add<AnyOfTrueSimplify>();
  bundle.add<DuplicateAllOfBranches>();
  bundle.add<DuplicateAnyOfBranches>();
  bundle.add<FlattenNestedAllOf>();
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
  if (mode != AlterSchemaMode::Canonicalizer) {
    bundle.add<ThenEmpty>();
    bundle.add<ElseEmpty>();
  }
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
  bundle.add<DraftRefSiblings>();
  bundle.add<DynamicRefToStaticRef>();
  bundle.add<UnknownKeywordsPrefix>();
  bundle.add<UnknownLocalRef>();
  bundle.add<RequiredPropertiesInProperties>();
  bundle.add<OrphanDefinitions>();

  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<ConstAsEnum>();
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<ExclusiveMaximumIntegerToMaximum>();
    bundle.add<ExclusiveMinimumIntegerToMinimum>();
    bundle.add<TypeBooleanAsEnum>();
    bundle.add<TypeNullAsEnum>();
    bundle.add<MaxContainsCoveredByMaxItems>();
    bundle.add<MinItemsGivenMinContains>();
    bundle.add<MinPropertiesCoveredByRequired>();
    bundle.add<MinLengthImplicit>();
    bundle.add<MultipleOfImplicit>();
    bundle.add<DivisibleByImplicit>();
    bundle.add<MaxDecimalImplicit>();
    bundle.add<ItemsImplicit>();
  }

  if (mode == AlterSchemaMode::Linter) {
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<ConstNotInEnum>();
    bundle.add<ContentSchemaDefault>();
    bundle.add<DependenciesDefault>();
    bundle.add<DependentRequiredDefault>();
    bundle.add<ItemsArrayDefault>();
    bundle.add<ItemsSchemaDefault>();
    bundle.add<MultipleOfDefault>();
    bundle.add<PatternPropertiesDefault>();
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
    bundle.add<DuplicateExamples>();
    bundle.add<SimplePropertiesIdentifiers>();
    bundle.add<InvalidExternalRef>();
    bundle.add<ValidDefault>();
    bundle.add<ValidExamples>();
  }

  if (mode != AlterSchemaMode::Canonicalizer) {
    bundle.add<UnnecessaryAllOfRefWrapperModern>();
  }
  bundle.add<UnnecessaryAllOfRefWrapperDraft>();

  if (mode != AlterSchemaMode::Canonicalizer) {
    bundle.add<UnnecessaryAllOfWrapper>();
  }

  bundle.add<DropAllOfEmptySchemas>();
  bundle.add<EmptyObjectAsTrue>();

  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<UnsatisfiableTypeAndEnum>();
    bundle.add<EnumFilterByType>();
    bundle.add<TypeUnionToSchemas>();
    bundle.add<DependenciesToAnyOf>();
    bundle.add<DependenciesToExtendsDisallow>();
    bundle.add<DependentSchemasToAnyOf>();
    bundle.add<DependentRequiredToAnyOf>();
    bundle.add<EnumDropRedundantValidation>();
    bundle.add<TypeWithApplicatorToAllOf>();
    bundle.add<TypeWithApplicatorToExtends>();
    bundle.add<EmptyDefinitionsDrop>();
    bundle.add<EmptyDefsDrop>();
    bundle.add<EmptyDependenciesDrop>();
    bundle.add<EmptyDependentSchemasDrop>();
    bundle.add<EmptyDependentRequiredDrop>();
    bundle.add<AdditionalItemsImplicit>();
    bundle.add<RequiredPropertyImplicit>();
    bundle.add<OptionalPropertyImplicit>();
    bundle.add<SingleBranchAllOf>();
    bundle.add<SingleBranchAnyOf>();
    bundle.add<SingleBranchOneOf>();
  }
}

} // namespace sourcemeta::blaze
