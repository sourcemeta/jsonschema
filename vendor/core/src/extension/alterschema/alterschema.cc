#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/regex.h>

// For built-in rules
#include <algorithm>     // std::sort, std::unique
#include <cmath>         // std::floor
#include <iterator>      // std::back_inserter
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
namespace sourcemeta::core {

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

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

// Canonicalizer
#include "canonicalizer/boolean_true.h"
#include "canonicalizer/const_as_enum.h"
#include "canonicalizer/exclusive_maximum_integer_to_maximum.h"
#include "canonicalizer/exclusive_minimum_integer_to_minimum.h"
#include "canonicalizer/max_contains_covered_by_max_items.h"
#include "canonicalizer/min_items_given_min_contains.h"
#include "canonicalizer/min_items_implicit.h"
#include "canonicalizer/min_length_implicit.h"
#include "canonicalizer/min_properties_covered_by_required.h"
#include "canonicalizer/min_properties_implicit.h"
#include "canonicalizer/multiple_of_implicit.h"
#include "canonicalizer/no_metadata.h"
#include "canonicalizer/properties_implicit.h"
#include "canonicalizer/type_array_to_any_of.h"
#include "canonicalizer/type_boolean_as_enum.h"
#include "canonicalizer/type_null_as_enum.h"
#include "canonicalizer/type_union_implicit.h"

// Common
#include "common/const_with_type.h"
#include "common/content_media_type_without_encoding.h"
#include "common/content_schema_without_media_type.h"
#include "common/dependencies_property_tautology.h"
#include "common/dependent_required_tautology.h"
#include "common/draft_official_dialect_without_empty_fragment.h"
#include "common/draft_ref_siblings.h"
#include "common/drop_allof_empty_schemas.h"
#include "common/duplicate_allof_branches.h"
#include "common/duplicate_anyof_branches.h"
#include "common/duplicate_enum_values.h"
#include "common/duplicate_required_values.h"
#include "common/else_empty.h"
#include "common/else_without_if.h"
#include "common/enum_with_type.h"
#include "common/equal_numeric_bounds_to_enum.h"
#include "common/exclusive_maximum_number_and_maximum.h"
#include "common/exclusive_minimum_number_and_minimum.h"
#include "common/if_without_then_else.h"
#include "common/ignored_metaschema.h"
#include "common/max_contains_without_contains.h"
#include "common/maximum_real_for_integer.h"
#include "common/min_contains_without_contains.h"
#include "common/minimum_real_for_integer.h"
#include "common/modern_official_dialect_with_empty_fragment.h"
#include "common/non_applicable_additional_items.h"
#include "common/non_applicable_enum_validation_keywords.h"
#include "common/non_applicable_type_specific_keywords.h"
#include "common/not_false.h"
#include "common/orphan_definitions.h"
#include "common/required_properties_in_properties.h"
#include "common/single_type_array.h"
#include "common/then_empty.h"
#include "common/then_without_if.h"
#include "common/unknown_keywords_prefix.h"
#include "common/unknown_local_ref.h"
#include "common/unnecessary_allof_ref_wrapper_draft.h"
#include "common/unnecessary_allof_ref_wrapper_modern.h"
#include "common/unnecessary_allof_wrapper.h"
#include "common/unsatisfiable_in_place_applicator_type.h"

// Linter
#include "linter/additional_properties_default.h"
#include "linter/comment_trim.h"
#include "linter/content_schema_default.h"
#include "linter/definitions_to_defs.h"
#include "linter/dependencies_default.h"
#include "linter/dependent_required_default.h"
#include "linter/description_trailing_period.h"
#include "linter/description_trim.h"
#include "linter/duplicate_examples.h"
#include "linter/enum_to_const.h"
#include "linter/equal_numeric_bounds_to_const.h"
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

#undef ONLY_CONTINUE_IF
} // namespace sourcemeta::core

namespace sourcemeta::core {

auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void {
  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<TypeUnionImplicit>();
    bundle.add<TypeArrayToAnyOf>();
  }

  if (mode == AlterSchemaMode::Linter) {
    bundle.add<DefinitionsToDefs>();
  }

  bundle.add<ContentMediaTypeWithoutEncoding>();
  bundle.add<ContentSchemaWithoutMediaType>();
  bundle.add<DraftOfficialDialectWithoutEmptyFragment>();
  bundle.add<NonApplicableTypeSpecificKeywords>();
  bundle.add<DuplicateAllOfBranches>();
  bundle.add<DuplicateAnyOfBranches>();
  bundle.add<UnsatisfiableInPlaceApplicatorType>();
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
  bundle.add<NonApplicableAdditionalItems>();
  bundle.add<ModernOfficialDialectWithEmptyFragment>();
  bundle.add<ExclusiveMaximumNumberAndMaximum>();
  bundle.add<ExclusiveMinimumNumberAndMinimum>();
  bundle.add<DraftRefSiblings>();
  bundle.add<UnknownKeywordsPrefix>();
  bundle.add<UnknownLocalRef>();
  bundle.add<RequiredPropertiesInProperties>();
  bundle.add<OrphanDefinitions>();

  if (mode == AlterSchemaMode::Canonicalizer) {
    bundle.add<BooleanTrue>();
    bundle.add<ConstAsEnum>();
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<ExclusiveMaximumIntegerToMaximum>();
    bundle.add<ExclusiveMinimumIntegerToMinimum>();
    bundle.add<TypeBooleanAsEnum>();
    bundle.add<TypeNullAsEnum>();
    bundle.add<MaxContainsCoveredByMaxItems>();
    bundle.add<MinItemsGivenMinContains>();
    bundle.add<MinPropertiesCoveredByRequired>();
    bundle.add<NoMetadata>();
    bundle.add<MinItemsImplicit>();
    bundle.add<MinLengthImplicit>();
    bundle.add<MinPropertiesImplicit>();
    bundle.add<MultipleOfImplicit>();
    bundle.add<PropertiesImplicit>();
  }

  if (mode == AlterSchemaMode::Linter) {
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<AdditionalPropertiesDefault>();
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
    bundle.add<UnsatisfiableMinProperties>();
    bundle.add<EnumToConst>();
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
  }

  bundle.add<UnnecessaryAllOfRefWrapperModern>();
  bundle.add<UnnecessaryAllOfRefWrapperDraft>();
  bundle.add<UnnecessaryAllOfWrapper>();
  bundle.add<DropAllOfEmptySchemas>();
}

} // namespace sourcemeta::core
