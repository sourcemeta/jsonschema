#include <sourcemeta/core/alterschema.h>

#include <cassert> // assert

// For built-in rules
#include <algorithm>
#include <cmath>
#include <iterator>
namespace sourcemeta::core {
static auto
contains_any(const Vocabularies &container,
             const std::set<typename Vocabularies::key_type> &values) -> bool {
  return std::any_of(std::cbegin(container), std::cend(container),
                     [&values](const auto &element) {
                       return values.contains(element.first);
                     });
}

template <typename T>
static auto every_item_is_null(const T &container) -> bool {
  return std::all_of(std::cbegin(container), std::cend(container),
                     [](const auto &element) { return element.is_null(); });
}

template <typename T>
static auto every_item_is_boolean(const T &container) -> bool {
  return std::all_of(std::cbegin(container), std::cend(container),
                     [](const auto &element) { return element.is_boolean(); });
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
#include "linter/additional_properties_default.h"
#include "linter/const_with_type.h"
#include "linter/content_media_type_without_encoding.h"
#include "linter/content_schema_default.h"
#include "linter/content_schema_without_media_type.h"
#include "linter/dependencies_default.h"
#include "linter/dependencies_property_tautology.h"
#include "linter/dependent_required_default.h"
#include "linter/dependent_required_tautology.h"
#include "linter/drop_non_array_keywords_applicator_2019_09.h"
#include "linter/drop_non_array_keywords_applicator_2020_12.h"
#include "linter/drop_non_array_keywords_content_2019_09.h"
#include "linter/drop_non_array_keywords_content_2020_12.h"
#include "linter/drop_non_array_keywords_draft0.h"
#include "linter/drop_non_array_keywords_draft1.h"
#include "linter/drop_non_array_keywords_draft2.h"
#include "linter/drop_non_array_keywords_draft3.h"
#include "linter/drop_non_array_keywords_draft4.h"
#include "linter/drop_non_array_keywords_draft6.h"
#include "linter/drop_non_array_keywords_draft7.h"
#include "linter/drop_non_array_keywords_format_2019_09.h"
#include "linter/drop_non_array_keywords_format_2020_12.h"
#include "linter/drop_non_array_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_array_keywords_validation_2019_09.h"
#include "linter/drop_non_array_keywords_validation_2020_12.h"
#include "linter/drop_non_boolean_keywords_applicator_2019_09.h"
#include "linter/drop_non_boolean_keywords_applicator_2020_12.h"
#include "linter/drop_non_boolean_keywords_content_2019_09.h"
#include "linter/drop_non_boolean_keywords_content_2020_12.h"
#include "linter/drop_non_boolean_keywords_draft0.h"
#include "linter/drop_non_boolean_keywords_draft1.h"
#include "linter/drop_non_boolean_keywords_draft2.h"
#include "linter/drop_non_boolean_keywords_draft3.h"
#include "linter/drop_non_boolean_keywords_draft4.h"
#include "linter/drop_non_boolean_keywords_draft6.h"
#include "linter/drop_non_boolean_keywords_draft7.h"
#include "linter/drop_non_boolean_keywords_format_2019_09.h"
#include "linter/drop_non_boolean_keywords_format_2020_12.h"
#include "linter/drop_non_boolean_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_boolean_keywords_validation_2019_09.h"
#include "linter/drop_non_boolean_keywords_validation_2020_12.h"
#include "linter/drop_non_null_keywords_applicator_2019_09.h"
#include "linter/drop_non_null_keywords_applicator_2020_12.h"
#include "linter/drop_non_null_keywords_content_2019_09.h"
#include "linter/drop_non_null_keywords_content_2020_12.h"
#include "linter/drop_non_null_keywords_draft0.h"
#include "linter/drop_non_null_keywords_draft1.h"
#include "linter/drop_non_null_keywords_draft2.h"
#include "linter/drop_non_null_keywords_draft3.h"
#include "linter/drop_non_null_keywords_draft4.h"
#include "linter/drop_non_null_keywords_draft6.h"
#include "linter/drop_non_null_keywords_draft7.h"
#include "linter/drop_non_null_keywords_format_2019_09.h"
#include "linter/drop_non_null_keywords_format_2020_12.h"
#include "linter/drop_non_null_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_null_keywords_validation_2019_09.h"
#include "linter/drop_non_null_keywords_validation_2020_12.h"
#include "linter/drop_non_numeric_keywords_applicator_2019_09.h"
#include "linter/drop_non_numeric_keywords_applicator_2020_12.h"
#include "linter/drop_non_numeric_keywords_content_2019_09.h"
#include "linter/drop_non_numeric_keywords_content_2020_12.h"
#include "linter/drop_non_numeric_keywords_draft0.h"
#include "linter/drop_non_numeric_keywords_draft1.h"
#include "linter/drop_non_numeric_keywords_draft2.h"
#include "linter/drop_non_numeric_keywords_draft3.h"
#include "linter/drop_non_numeric_keywords_draft4.h"
#include "linter/drop_non_numeric_keywords_draft6.h"
#include "linter/drop_non_numeric_keywords_draft7.h"
#include "linter/drop_non_numeric_keywords_format_2019_09.h"
#include "linter/drop_non_numeric_keywords_format_2020_12.h"
#include "linter/drop_non_numeric_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_numeric_keywords_validation_2019_09.h"
#include "linter/drop_non_numeric_keywords_validation_2020_12.h"
#include "linter/drop_non_object_keywords_applicator_2019_09.h"
#include "linter/drop_non_object_keywords_applicator_2020_12.h"
#include "linter/drop_non_object_keywords_content_2019_09.h"
#include "linter/drop_non_object_keywords_content_2020_12.h"
#include "linter/drop_non_object_keywords_draft0.h"
#include "linter/drop_non_object_keywords_draft1.h"
#include "linter/drop_non_object_keywords_draft2.h"
#include "linter/drop_non_object_keywords_draft3.h"
#include "linter/drop_non_object_keywords_draft4.h"
#include "linter/drop_non_object_keywords_draft6.h"
#include "linter/drop_non_object_keywords_draft7.h"
#include "linter/drop_non_object_keywords_format_2019_09.h"
#include "linter/drop_non_object_keywords_format_2020_12.h"
#include "linter/drop_non_object_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_object_keywords_validation_2019_09.h"
#include "linter/drop_non_object_keywords_validation_2020_12.h"
#include "linter/drop_non_string_keywords_applicator_2019_09.h"
#include "linter/drop_non_string_keywords_applicator_2020_12.h"
#include "linter/drop_non_string_keywords_draft0.h"
#include "linter/drop_non_string_keywords_draft1.h"
#include "linter/drop_non_string_keywords_draft2.h"
#include "linter/drop_non_string_keywords_draft3.h"
#include "linter/drop_non_string_keywords_draft4.h"
#include "linter/drop_non_string_keywords_draft6.h"
#include "linter/drop_non_string_keywords_draft7.h"
#include "linter/drop_non_string_keywords_unevaluated_2020_12.h"
#include "linter/drop_non_string_keywords_validation_2019_09.h"
#include "linter/drop_non_string_keywords_validation_2020_12.h"
#include "linter/duplicate_allof_branches.h"
#include "linter/duplicate_anyof_branches.h"
#include "linter/duplicate_enum_values.h"
#include "linter/duplicate_required_values.h"
#include "linter/else_without_if.h"
#include "linter/enum_to_const.h"
#include "linter/enum_with_type.h"
#include "linter/equal_numeric_bounds_to_enum.h"
#include "linter/exclusive_maximum_number_and_maximum.h"
#include "linter/exclusive_minimum_number_and_minimum.h"
#include "linter/if_without_then_else.h"
#include "linter/items_array_default.h"
#include "linter/items_schema_default.h"
#include "linter/max_contains_without_contains.h"
#include "linter/maximum_real_for_integer.h"
#include "linter/min_contains_without_contains.h"
#include "linter/minimum_real_for_integer.h"
#include "linter/pattern_properties_default.h"
#include "linter/properties_default.h"
#include "linter/single_type_array.h"
#include "linter/then_without_if.h"
#include "linter/unevaluated_items_default.h"
#include "linter/unevaluated_properties_default.h"
#include "linter/unnecessary_allof_ref_wrapper.h"
#include "linter/unsatisfiable_max_contains.h"
#include "linter/unsatisfiable_min_properties.h"
} // namespace sourcemeta::core

namespace sourcemeta::core {

auto add(SchemaTransformer &bundle, const AlterSchemaMode mode)

    -> void {
  // Common rules that apply to all modes
  bundle.add<ContentMediaTypeWithoutEncoding>();
  bundle.add<ContentSchemaWithoutMediaType>();
  bundle.add<DropNonArrayKeywordsApplicator_2019_09>();
  bundle.add<DropNonArrayKeywordsApplicator_2020_12>();
  bundle.add<DropNonArrayKeywordsContent_2019_09>();
  bundle.add<DropNonArrayKeywordsContent_2020_12>();
  bundle.add<DropNonArrayKeywords_Draft0>();
  bundle.add<DropNonArrayKeywords_Draft1>();
  bundle.add<DropNonArrayKeywords_Draft2>();
  bundle.add<DropNonArrayKeywords_Draft3>();
  bundle.add<DropNonArrayKeywords_Draft4>();
  bundle.add<DropNonArrayKeywords_Draft6>();
  bundle.add<DropNonArrayKeywords_Draft7>();
  bundle.add<DropNonArrayKeywordsFormat_2019_09>();
  bundle.add<DropNonArrayKeywordsFormat_2020_12>();
  bundle.add<DropNonArrayKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonArrayKeywordsValidation_2019_09>();
  bundle.add<DropNonArrayKeywordsValidation_2020_12>();
  bundle.add<DropNonBooleanKeywordsApplicator_2019_09>();
  bundle.add<DropNonBooleanKeywordsApplicator_2020_12>();
  bundle.add<DropNonBooleanKeywordsContent_2019_09>();
  bundle.add<DropNonBooleanKeywordsContent_2020_12>();
  bundle.add<DropNonBooleanKeywords_Draft0>();
  bundle.add<DropNonBooleanKeywords_Draft1>();
  bundle.add<DropNonBooleanKeywords_Draft2>();
  bundle.add<DropNonBooleanKeywords_Draft3>();
  bundle.add<DropNonBooleanKeywords_Draft4>();
  bundle.add<DropNonBooleanKeywords_Draft6>();
  bundle.add<DropNonBooleanKeywords_Draft7>();
  bundle.add<DropNonBooleanKeywordsFormat_2019_09>();
  bundle.add<DropNonBooleanKeywordsFormat_2020_12>();
  bundle.add<DropNonBooleanKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonBooleanKeywordsValidation_2019_09>();
  bundle.add<DropNonBooleanKeywordsValidation_2020_12>();
  bundle.add<DropNonNullKeywordsApplicator_2019_09>();
  bundle.add<DropNonNullKeywordsApplicator_2020_12>();
  bundle.add<DropNonNullKeywordsContent_2019_09>();
  bundle.add<DropNonNullKeywordsContent_2020_12>();
  bundle.add<DropNonNullKeywords_Draft0>();
  bundle.add<DropNonNullKeywords_Draft1>();
  bundle.add<DropNonNullKeywords_Draft2>();
  bundle.add<DropNonNullKeywords_Draft3>();
  bundle.add<DropNonNullKeywords_Draft4>();
  bundle.add<DropNonNullKeywords_Draft6>();
  bundle.add<DropNonNullKeywords_Draft7>();
  bundle.add<DropNonNullKeywordsFormat_2019_09>();
  bundle.add<DropNonNullKeywordsFormat_2020_12>();
  bundle.add<DropNonNullKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonNullKeywordsValidation_2019_09>();
  bundle.add<DropNonNullKeywordsValidation_2020_12>();
  bundle.add<DropNonNumericKeywordsApplicator_2019_09>();
  bundle.add<DropNonNumericKeywordsApplicator_2020_12>();
  bundle.add<DropNonNumericKeywordsContent_2019_09>();
  bundle.add<DropNonNumericKeywordsContent_2020_12>();
  bundle.add<DropNonNumericKeywords_Draft0>();
  bundle.add<DropNonNumericKeywords_Draft1>();
  bundle.add<DropNonNumericKeywords_Draft2>();
  bundle.add<DropNonNumericKeywords_Draft3>();
  bundle.add<DropNonNumericKeywords_Draft4>();
  bundle.add<DropNonNumericKeywords_Draft6>();
  bundle.add<DropNonNumericKeywords_Draft7>();
  bundle.add<DropNonNumericKeywordsFormat_2019_09>();
  bundle.add<DropNonNumericKeywordsFormat_2020_12>();
  bundle.add<DropNonNumericKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonNumericKeywordsValidation_2019_09>();
  bundle.add<DropNonNumericKeywordsValidation_2020_12>();
  bundle.add<DropNonObjectKeywordsApplicator_2019_09>();
  bundle.add<DropNonObjectKeywordsApplicator_2020_12>();
  bundle.add<DropNonObjectKeywordsContent_2019_09>();
  bundle.add<DropNonObjectKeywordsContent_2020_12>();
  bundle.add<DropNonObjectKeywords_Draft0>();
  bundle.add<DropNonObjectKeywords_Draft1>();
  bundle.add<DropNonObjectKeywords_Draft2>();
  bundle.add<DropNonObjectKeywords_Draft3>();
  bundle.add<DropNonObjectKeywords_Draft4>();
  bundle.add<DropNonObjectKeywords_Draft6>();
  bundle.add<DropNonObjectKeywords_Draft7>();
  bundle.add<DropNonObjectKeywordsFormat_2019_09>();
  bundle.add<DropNonObjectKeywordsFormat_2020_12>();
  bundle.add<DropNonObjectKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonObjectKeywordsValidation_2019_09>();
  bundle.add<DropNonObjectKeywordsValidation_2020_12>();
  bundle.add<DropNonStringKeywordsApplicator_2019_09>();
  bundle.add<DropNonStringKeywordsApplicator_2020_12>();
  bundle.add<DropNonStringKeywords_Draft0>();
  bundle.add<DropNonStringKeywords_Draft1>();
  bundle.add<DropNonStringKeywords_Draft2>();
  bundle.add<DropNonStringKeywords_Draft3>();
  bundle.add<DropNonStringKeywords_Draft4>();
  bundle.add<DropNonStringKeywords_Draft6>();
  bundle.add<DropNonStringKeywords_Draft7>();
  bundle.add<DropNonStringKeywordsUnevaluated_2020_12>();
  bundle.add<DropNonStringKeywordsValidation_2019_09>();
  bundle.add<DropNonStringKeywordsValidation_2020_12>();
  bundle.add<UnnecessaryAllOfRefWrapper>();
  bundle.add<DuplicateAllOfBranches>();
  bundle.add<DuplicateAnyOfBranches>();
  bundle.add<ElseWithoutIf>();
  bundle.add<IfWithoutThenElse>();
  bundle.add<MaxContainsWithoutContains>();
  bundle.add<MinContainsWithoutContains>();
  bundle.add<ThenWithoutIf>();
  bundle.add<DependenciesPropertyTautology>();
  bundle.add<DependentRequiredTautology>();
  bundle.add<EqualNumericBoundsToEnum>();
  bundle.add<MaximumRealForInteger>();
  bundle.add<MinimumRealForInteger>();
  bundle.add<SingleTypeArray>();
  bundle.add<EnumWithType>();
  bundle.add<DuplicateEnumValues>();
  bundle.add<DuplicateRequiredValues>();
  bundle.add<ConstWithType>();
  bundle.add<ExclusiveMaximumNumberAndMaximum>();
  bundle.add<ExclusiveMinimumNumberAndMinimum>();

  switch (mode) {
    case AlterSchemaMode::StaticAnalysis:
      bundle.add<BooleanTrue>();
      bundle.add<ConstAsEnum>();
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
      break;
    case AlterSchemaMode::Readability:
      bundle.add<AdditionalPropertiesDefault>();
      bundle.add<ContentSchemaDefault>();
      bundle.add<DependenciesDefault>();
      bundle.add<DependentRequiredDefault>();
      bundle.add<ItemsArrayDefault>();
      bundle.add<ItemsSchemaDefault>();
      bundle.add<PatternPropertiesDefault>();
      bundle.add<PropertiesDefault>();
      bundle.add<UnevaluatedItemsDefault>();
      bundle.add<UnevaluatedPropertiesDefault>();
      bundle.add<UnsatisfiableMaxContains>();
      bundle.add<UnsatisfiableMinProperties>();
      bundle.add<EnumToConst>();
      break;
    default:
      // We should never get here
      assert(false);
      break;
  }
}

} // namespace sourcemeta::core
