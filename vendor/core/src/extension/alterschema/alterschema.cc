#include <sourcemeta/core/alterschema.h>

#include <cassert> // assert

// For built-in rules
#include <algorithm>
#include <cmath>
#include <iterator>
namespace sourcemeta::core {
auto contains_any(const Vocabularies &container,
                  const std::set<typename Vocabularies::key_type> &values)
    -> bool {
  return std::any_of(std::cbegin(container), std::cend(container),
                     [&values](const auto &element) {
                       return values.contains(element.first);
                     });
}

template <typename T> auto every_item_is_null(const T &container) -> bool {
  return std::all_of(std::cbegin(container), std::cend(container),
                     [](const auto &element) { return element.is_null(); });
}

template <typename T> auto every_item_is_boolean(const T &container) -> bool {
  return std::all_of(std::cbegin(container), std::cend(container),
                     [](const auto &element) { return element.is_boolean(); });
}

// AntiPattern
#include "antipattern/const_with_type.h"
#include "antipattern/duplicate_enum_values.h"
#include "antipattern/duplicate_required_values.h"
#include "antipattern/enum_with_type.h"
#include "antipattern/exclusive_maximum_number_and_maximum.h"
#include "antipattern/exclusive_minimum_number_and_minimum.h"
// Simplify
#include "simplify/dependencies_property_tautology.h"
#include "simplify/dependent_required_tautology.h"
#include "simplify/equal_numeric_bounds_to_enum.h"
#include "simplify/maximum_real_for_integer.h"
#include "simplify/minimum_real_for_integer.h"
#include "simplify/single_type_array.h"
// Syntax sugar
#include "syntax_sugar/enum_to_const.h"
// Desugar
#include "desugar/boolean_true.h"
#include "desugar/const_as_enum.h"
#include "desugar/exclusive_maximum_integer_to_maximum.h"
#include "desugar/exclusive_minimum_integer_to_minimum.h"
#include "desugar/type_array_to_any_of_2020_12.h"
#include "desugar/type_boolean_as_enum.h"
#include "desugar/type_null_as_enum.h"

// Redundant
#include "redundant/additional_properties_default.h"
#include "redundant/content_schema_default.h"
#include "redundant/dependencies_default.h"
#include "redundant/dependent_required_default.h"
#include "redundant/items_array_default.h"
#include "redundant/items_schema_default.h"
#include "redundant/pattern_properties_default.h"
#include "redundant/properties_default.h"
#include "redundant/unevaluated_items_default.h"
#include "redundant/unevaluated_properties_default.h"
#include "redundant/unsatisfiable_max_contains.h"
#include "redundant/unsatisfiable_min_properties.h"
// Implicit
#include "implicit/max_contains_covered_by_max_items.h"
#include "implicit/min_items_given_min_contains.h"
#include "implicit/min_items_implicit.h"
#include "implicit/min_length_implicit.h"
#include "implicit/min_properties_covered_by_required.h"
#include "implicit/min_properties_implicit.h"
#include "implicit/multiple_of_implicit.h"
#include "implicit/properties_implicit.h"
#include "implicit/type_union_implicit.h"
// Superfluous
#include "superfluous/content_media_type_without_encoding.h"
#include "superfluous/content_schema_without_media_type.h"
#include "superfluous/drop_non_array_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_array_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_array_keywords_content_2019_09.h"
#include "superfluous/drop_non_array_keywords_content_2020_12.h"
#include "superfluous/drop_non_array_keywords_draft0.h"
#include "superfluous/drop_non_array_keywords_draft1.h"
#include "superfluous/drop_non_array_keywords_draft2.h"
#include "superfluous/drop_non_array_keywords_draft3.h"
#include "superfluous/drop_non_array_keywords_draft4.h"
#include "superfluous/drop_non_array_keywords_draft6.h"
#include "superfluous/drop_non_array_keywords_draft7.h"
#include "superfluous/drop_non_array_keywords_format_2019_09.h"
#include "superfluous/drop_non_array_keywords_format_2020_12.h"
#include "superfluous/drop_non_array_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_array_keywords_validation_2019_09.h"
#include "superfluous/drop_non_array_keywords_validation_2020_12.h"
#include "superfluous/drop_non_boolean_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_boolean_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_boolean_keywords_content_2019_09.h"
#include "superfluous/drop_non_boolean_keywords_content_2020_12.h"
#include "superfluous/drop_non_boolean_keywords_draft0.h"
#include "superfluous/drop_non_boolean_keywords_draft1.h"
#include "superfluous/drop_non_boolean_keywords_draft2.h"
#include "superfluous/drop_non_boolean_keywords_draft3.h"
#include "superfluous/drop_non_boolean_keywords_draft4.h"
#include "superfluous/drop_non_boolean_keywords_draft6.h"
#include "superfluous/drop_non_boolean_keywords_draft7.h"
#include "superfluous/drop_non_boolean_keywords_format_2019_09.h"
#include "superfluous/drop_non_boolean_keywords_format_2020_12.h"
#include "superfluous/drop_non_boolean_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_boolean_keywords_validation_2019_09.h"
#include "superfluous/drop_non_boolean_keywords_validation_2020_12.h"
#include "superfluous/drop_non_null_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_null_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_null_keywords_content_2019_09.h"
#include "superfluous/drop_non_null_keywords_content_2020_12.h"
#include "superfluous/drop_non_null_keywords_draft0.h"
#include "superfluous/drop_non_null_keywords_draft1.h"
#include "superfluous/drop_non_null_keywords_draft2.h"
#include "superfluous/drop_non_null_keywords_draft3.h"
#include "superfluous/drop_non_null_keywords_draft4.h"
#include "superfluous/drop_non_null_keywords_draft6.h"
#include "superfluous/drop_non_null_keywords_draft7.h"
#include "superfluous/drop_non_null_keywords_format_2019_09.h"
#include "superfluous/drop_non_null_keywords_format_2020_12.h"
#include "superfluous/drop_non_null_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_null_keywords_validation_2019_09.h"
#include "superfluous/drop_non_null_keywords_validation_2020_12.h"
#include "superfluous/drop_non_numeric_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_numeric_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_numeric_keywords_content_2019_09.h"
#include "superfluous/drop_non_numeric_keywords_content_2020_12.h"
#include "superfluous/drop_non_numeric_keywords_draft0.h"
#include "superfluous/drop_non_numeric_keywords_draft1.h"
#include "superfluous/drop_non_numeric_keywords_draft2.h"
#include "superfluous/drop_non_numeric_keywords_draft3.h"
#include "superfluous/drop_non_numeric_keywords_draft4.h"
#include "superfluous/drop_non_numeric_keywords_draft6.h"
#include "superfluous/drop_non_numeric_keywords_draft7.h"
#include "superfluous/drop_non_numeric_keywords_format_2019_09.h"
#include "superfluous/drop_non_numeric_keywords_format_2020_12.h"
#include "superfluous/drop_non_numeric_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_numeric_keywords_validation_2019_09.h"
#include "superfluous/drop_non_numeric_keywords_validation_2020_12.h"
#include "superfluous/drop_non_object_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_object_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_object_keywords_content_2019_09.h"
#include "superfluous/drop_non_object_keywords_content_2020_12.h"
#include "superfluous/drop_non_object_keywords_draft0.h"
#include "superfluous/drop_non_object_keywords_draft1.h"
#include "superfluous/drop_non_object_keywords_draft2.h"
#include "superfluous/drop_non_object_keywords_draft3.h"
#include "superfluous/drop_non_object_keywords_draft4.h"
#include "superfluous/drop_non_object_keywords_draft6.h"
#include "superfluous/drop_non_object_keywords_draft7.h"
#include "superfluous/drop_non_object_keywords_format_2019_09.h"
#include "superfluous/drop_non_object_keywords_format_2020_12.h"
#include "superfluous/drop_non_object_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_object_keywords_validation_2019_09.h"
#include "superfluous/drop_non_object_keywords_validation_2020_12.h"
#include "superfluous/drop_non_string_keywords_applicator_2019_09.h"
#include "superfluous/drop_non_string_keywords_applicator_2020_12.h"
#include "superfluous/drop_non_string_keywords_draft0.h"
#include "superfluous/drop_non_string_keywords_draft1.h"
#include "superfluous/drop_non_string_keywords_draft2.h"
#include "superfluous/drop_non_string_keywords_draft3.h"
#include "superfluous/drop_non_string_keywords_draft4.h"
#include "superfluous/drop_non_string_keywords_draft6.h"
#include "superfluous/drop_non_string_keywords_draft7.h"
#include "superfluous/drop_non_string_keywords_unevaluated_2020_12.h"
#include "superfluous/drop_non_string_keywords_validation_2019_09.h"
#include "superfluous/drop_non_string_keywords_validation_2020_12.h"
#include "superfluous/duplicate_allof_branches.h"
#include "superfluous/duplicate_anyof_branches.h"
#include "superfluous/else_without_if.h"
#include "superfluous/if_without_then_else.h"
#include "superfluous/max_contains_without_contains.h"
#include "superfluous/min_contains_without_contains.h"
#include "superfluous/then_without_if.h"
} // namespace sourcemeta::core

namespace sourcemeta::core {

auto add(SchemaTransformer &bundle, const AlterSchemaCategory category)
    -> void {
  switch (category) {
    case AlterSchemaCategory::AntiPattern:
      bundle.add<EnumWithType>();
      bundle.add<DuplicateEnumValues>();
      bundle.add<DuplicateRequiredValues>();
      bundle.add<ConstWithType>();
      bundle.add<ExclusiveMaximumNumberAndMaximum>();
      bundle.add<ExclusiveMinimumNumberAndMinimum>();
      break;
    case AlterSchemaCategory::Simplify:
      bundle.add<DependenciesPropertyTautology>();
      bundle.add<DependentRequiredTautology>();
      bundle.add<EqualNumericBoundsToEnum>();
      bundle.add<MaximumRealForInteger>();
      bundle.add<MinimumRealForInteger>();
      bundle.add<SingleTypeArray>();
      break;
    case AlterSchemaCategory::SyntaxSugar:
      bundle.add<EnumToConst>();
      break;
    case AlterSchemaCategory::Desugar:
      bundle.add<BooleanTrue>();
      bundle.add<ConstAsEnum>();
      bundle.add<ExclusiveMaximumIntegerToMaximum>();
      bundle.add<ExclusiveMinimumIntegerToMinimum>();
      bundle.add<TypeArrayToAnyOf_2020_12>();
      bundle.add<TypeBooleanAsEnum>();
      bundle.add<TypeNullAsEnum>();
      break;
    case AlterSchemaCategory::Redundant:
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
      break;
    case AlterSchemaCategory::Implicit:
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
    case AlterSchemaCategory::Superfluous:
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
      bundle.add<DuplicateAllOfBranches>();
      bundle.add<DuplicateAnyOfBranches>();
      bundle.add<ElseWithoutIf>();
      bundle.add<IfWithoutThenElse>();
      bundle.add<MaxContainsWithoutContains>();
      bundle.add<MinContainsWithoutContains>();
      bundle.add<ThenWithoutIf>();
      break;
    default:
      // We should never get here
      assert(false);
      break;
  }
}

} // namespace sourcemeta::core
