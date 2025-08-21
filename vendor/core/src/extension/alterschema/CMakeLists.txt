sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME alterschema
  SOURCES alterschema.cc
    # Canonicalizer
    canonicalizer/max_contains_covered_by_max_items.h
    canonicalizer/min_items_given_min_contains.h
    canonicalizer/min_items_implicit.h
    canonicalizer/min_length_implicit.h
    canonicalizer/min_properties_covered_by_required.h
    canonicalizer/min_properties_implicit.h
    canonicalizer/multiple_of_implicit.h
    canonicalizer/properties_implicit.h
    canonicalizer/type_union_implicit.h
    canonicalizer/boolean_true.h
    canonicalizer/const_as_enum.h
    canonicalizer/exclusive_maximum_integer_to_maximum.h
    canonicalizer/exclusive_minimum_integer_to_minimum.h
    canonicalizer/type_array_to_any_of_2020_12.h
    canonicalizer/type_boolean_as_enum.h
    canonicalizer/type_null_as_enum.h

    # Linter
    linter/const_with_type.h
    linter/duplicate_enum_values.h
    linter/duplicate_required_values.h
    linter/exclusive_maximum_number_and_maximum.h
    linter/exclusive_minimum_number_and_minimum.h
    linter/enum_with_type.h
    linter/dependencies_property_tautology.h
    linter/dependent_required_tautology.h
    linter/equal_numeric_bounds_to_enum.h
    linter/maximum_real_for_integer.h
    linter/draft_official_dialect_without_empty_fragment.h
    linter/minimum_real_for_integer.h
    linter/single_type_array.h
    linter/enum_to_const.h
    linter/additional_properties_default.h
    linter/content_schema_default.h
    linter/dependencies_default.h
    linter/dependent_required_default.h
    linter/items_array_default.h
    linter/items_schema_default.h
    linter/multiple_of_default.h
    linter/pattern_properties_default.h
    linter/properties_default.h
    linter/unevaluated_items_default.h
    linter/unevaluated_properties_default.h
    linter/unsatisfiable_max_contains.h
    linter/unsatisfiable_min_properties.h
    linter/content_media_type_without_encoding.h
    linter/content_schema_without_media_type.h
    linter/additional_items_with_schema_items.h
    linter/non_applicable_type_specific_keywords.h
    linter/unnecessary_allof_wrapper_modern.h
    linter/unnecessary_allof_wrapper_draft.h
    linter/unnecessary_allof_wrapper_properties.h
    linter/duplicate_allof_branches.h
    linter/duplicate_anyof_branches.h
    linter/else_without_if.h
    linter/if_without_then_else.h
    linter/max_contains_without_contains.h
    linter/min_contains_without_contains.h
    linter/modern_official_dialect_with_empty_fragment.h
    linter/then_empty.h
    linter/else_empty.h
    linter/then_without_if.h
    linter/property_names_type_default.h
    linter/property_names_default.h
    linter/draft_ref_siblings.h
    linter/definitions_to_defs.h

    # Strict
    strict/required_properties_in_properties.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME alterschema)
endif()

target_link_libraries(sourcemeta_core_alterschema PUBLIC
  sourcemeta::core::jsonschema)
