#include <sourcemeta/core/alterschema.h>

// For built-in rules
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
namespace sourcemeta::core {
static auto
contains_any(const Vocabularies &container,
             const std::set<typename Vocabularies::key_type> &values) -> bool {
  return std::ranges::any_of(container, [&values](const auto &element) {
    return values.contains(element.first);
  });
}

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
#include "canonicalizer/properties_implicit.h"
#include "canonicalizer/type_array_to_any_of_2020_12.h"
#include "canonicalizer/type_boolean_as_enum.h"
#include "canonicalizer/type_null_as_enum.h"
#include "canonicalizer/type_union_implicit.h"

// Linter
#include "linter/additional_items_with_schema_items.h"
#include "linter/additional_properties_default.h"
#include "linter/const_with_type.h"
#include "linter/content_media_type_without_encoding.h"
#include "linter/content_schema_default.h"
#include "linter/content_schema_without_media_type.h"
#include "linter/definitions_to_defs.h"
#include "linter/dependencies_default.h"
#include "linter/dependencies_property_tautology.h"
#include "linter/dependent_required_default.h"
#include "linter/dependent_required_tautology.h"
#include "linter/draft_official_dialect_without_empty_fragment.h"
#include "linter/draft_ref_siblings.h"
#include "linter/duplicate_allof_branches.h"
#include "linter/duplicate_anyof_branches.h"
#include "linter/duplicate_enum_values.h"
#include "linter/duplicate_required_values.h"
#include "linter/else_empty.h"
#include "linter/else_without_if.h"
#include "linter/enum_to_const.h"
#include "linter/enum_with_type.h"
#include "linter/equal_numeric_bounds_to_const.h"
#include "linter/equal_numeric_bounds_to_enum.h"
#include "linter/exclusive_maximum_number_and_maximum.h"
#include "linter/exclusive_minimum_number_and_minimum.h"
#include "linter/if_without_then_else.h"
#include "linter/ignored_metaschema.h"
#include "linter/items_array_default.h"
#include "linter/items_schema_default.h"
#include "linter/max_contains_without_contains.h"
#include "linter/maximum_real_for_integer.h"
#include "linter/min_contains_without_contains.h"
#include "linter/minimum_real_for_integer.h"
#include "linter/modern_official_dialect_with_empty_fragment.h"
#include "linter/multiple_of_default.h"
#include "linter/non_applicable_enum_validation_keywords.h"
#include "linter/non_applicable_type_specific_keywords.h"
#include "linter/not_false.h"
#include "linter/pattern_properties_default.h"
#include "linter/properties_default.h"
#include "linter/property_names_default.h"
#include "linter/property_names_type_default.h"
#include "linter/single_type_array.h"
#include "linter/then_empty.h"
#include "linter/then_without_if.h"
#include "linter/unevaluated_items_default.h"
#include "linter/unevaluated_properties_default.h"
#include "linter/unknown_keywords_prefix.h"
#include "linter/unknown_local_ref.h"
#include "linter/unnecessary_allof_ref_wrapper_draft.h"
#include "linter/unnecessary_allof_ref_wrapper_modern.h"
#include "linter/unsatisfiable_max_contains.h"
#include "linter/unsatisfiable_min_properties.h"

// Strict
#include "strict/required_properties_in_properties.h"

#undef ONLY_CONTINUE_IF
} // namespace sourcemeta::core

namespace sourcemeta::core {

auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void {
  // Common rules that apply to all modes
  bundle.add<ContentMediaTypeWithoutEncoding>();
  bundle.add<ContentSchemaWithoutMediaType>();
  bundle.add<DraftOfficialDialectWithoutEmptyFragment>();
  bundle.add<NonApplicableTypeSpecificKeywords>();
  bundle.add<UnnecessaryAllOfRefWrapperModern>();
  bundle.add<UnnecessaryAllOfRefWrapperDraft>();
  bundle.add<DuplicateAllOfBranches>();
  bundle.add<DuplicateAnyOfBranches>();
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
  bundle.add<AdditionalItemsWithSchemaItems>();
  bundle.add<ModernOfficialDialectWithEmptyFragment>();
  bundle.add<ExclusiveMaximumNumberAndMaximum>();
  bundle.add<ExclusiveMinimumNumberAndMinimum>();
  bundle.add<DraftRefSiblings>();
  bundle.add<UnknownKeywordsPrefix>();
  bundle.add<UnknownLocalRef>();

  if (mode == AlterSchemaMode::StaticAnalysis) {
    bundle.add<BooleanTrue>();
    bundle.add<ConstAsEnum>();
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<ExclusiveMaximumIntegerToMaximum>();
    bundle.add<ExclusiveMinimumIntegerToMinimum>();
    bundle.add<TypeArrayToAnyOf_2020_12>();
    bundle.add<TypeBooleanAsEnum>();
    bundle.add<TypeNullAsEnum>();
    bundle.add<MaxContainsCoveredByMaxItems>();
    bundle.add<MinItemsGivenMinContains>();
    bundle.add<MinItemsImplicit>();
    bundle.add<MinLengthImplicit>();
    bundle.add<MinPropertiesCoveredByRequired>();
    bundle.add<MinPropertiesImplicit>();
    bundle.add<MultipleOfImplicit>();
    bundle.add<PropertiesImplicit>();
    bundle.add<TypeUnionImplicit>();
  }

  if (mode == AlterSchemaMode::Readability ||
      mode == AlterSchemaMode::ReadabilityStrict) {
    bundle.add<EqualNumericBoundsToConst>();
    bundle.add<AdditionalPropertiesDefault>();
    bundle.add<ContentSchemaDefault>();
    bundle.add<DefinitionsToDefs>();
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
  }

  if (mode == AlterSchemaMode::ReadabilityStrict) {
    bundle.add<RequiredPropertiesInProperties>();
  }
}

} // namespace sourcemeta::core
