sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME alterschema
  FOLDER "Core/AlterSchema"
  SOURCES alterschema.cc
    # Antipattern
    antipattern/const_with_type.h
    antipattern/duplicate_enum_values.h
    antipattern/duplicate_required_values.h
    antipattern/exclusive_maximum_number_and_maximum.h
    antipattern/exclusive_minimum_number_and_minimum.h
    antipattern/enum_with_type.h

    # Simplify
    simplify/dependencies_property_tautology.h
    simplify/dependent_required_tautology.h
    simplify/equal_numeric_bounds_to_enum.h
    simplify/maximum_real_for_integer.h
    simplify/minimum_real_for_integer.h
    simplify/single_type_array.h

    # Syntax Sugar
    syntax_sugar/enum_to_const.h

    # Desugar
    desugar/boolean_true.h
    desugar/const_as_enum.h
    desugar/exclusive_maximum_integer_to_maximum.h
    desugar/exclusive_minimum_integer_to_minimum.h
    desugar/type_array_to_any_of_2020_12.h
    desugar/type_boolean_as_enum.h
    desugar/type_null_as_enum.h

    # Redundant
    redundant/additional_properties_default.h
    redundant/content_schema_default.h
    redundant/dependencies_default.h
    redundant/dependent_required_default.h
    redundant/items_array_default.h
    redundant/items_schema_default.h
    redundant/pattern_properties_default.h
    redundant/properties_default.h
    redundant/unevaluated_items_default.h
    redundant/unevaluated_properties_default.h
    redundant/unsatisfiable_max_contains.h
    redundant/unsatisfiable_min_properties.h

    # Implicit
    implicit/max_contains_covered_by_max_items.h
    implicit/min_items_given_min_contains.h
    implicit/min_items_implicit.h
    implicit/min_length_implicit.h
    implicit/min_properties_covered_by_required.h
    implicit/min_properties_implicit.h
    implicit/multiple_of_implicit.h
    implicit/properties_implicit.h
    implicit/type_union_implicit.h

    # Superfluous
    superfluous/content_media_type_without_encoding.h
    superfluous/content_schema_without_media_type.h
    superfluous/drop_non_array_keywords_applicator_2019_09.h
    superfluous/drop_non_array_keywords_applicator_2020_12.h
    superfluous/drop_non_array_keywords_content_2019_09.h
    superfluous/drop_non_array_keywords_content_2020_12.h
    superfluous/drop_non_array_keywords_draft0.h
    superfluous/drop_non_array_keywords_draft1.h
    superfluous/drop_non_array_keywords_draft2.h
    superfluous/drop_non_array_keywords_draft3.h
    superfluous/drop_non_array_keywords_draft4.h
    superfluous/drop_non_array_keywords_draft6.h
    superfluous/drop_non_array_keywords_draft7.h
    superfluous/drop_non_array_keywords_format_2019_09.h
    superfluous/drop_non_array_keywords_format_2020_12.h
    superfluous/drop_non_array_keywords_unevaluated_2020_12.h
    superfluous/drop_non_array_keywords_validation_2019_09.h
    superfluous/drop_non_array_keywords_validation_2020_12.h
    superfluous/drop_non_boolean_keywords_applicator_2019_09.h
    superfluous/drop_non_boolean_keywords_applicator_2020_12.h
    superfluous/drop_non_boolean_keywords_content_2019_09.h
    superfluous/drop_non_boolean_keywords_content_2020_12.h
    superfluous/drop_non_boolean_keywords_draft0.h
    superfluous/drop_non_boolean_keywords_draft1.h
    superfluous/drop_non_boolean_keywords_draft2.h
    superfluous/drop_non_boolean_keywords_draft3.h
    superfluous/drop_non_boolean_keywords_draft4.h
    superfluous/drop_non_boolean_keywords_draft6.h
    superfluous/drop_non_boolean_keywords_draft7.h
    superfluous/drop_non_boolean_keywords_format_2019_09.h
    superfluous/drop_non_boolean_keywords_format_2020_12.h
    superfluous/drop_non_boolean_keywords_unevaluated_2020_12.h
    superfluous/drop_non_boolean_keywords_validation_2019_09.h
    superfluous/drop_non_boolean_keywords_validation_2020_12.h
    superfluous/drop_non_null_keywords_applicator_2019_09.h
    superfluous/drop_non_null_keywords_applicator_2020_12.h
    superfluous/drop_non_null_keywords_content_2019_09.h
    superfluous/drop_non_null_keywords_content_2020_12.h
    superfluous/drop_non_null_keywords_draft0.h
    superfluous/drop_non_null_keywords_draft1.h
    superfluous/drop_non_null_keywords_draft2.h
    superfluous/drop_non_null_keywords_draft3.h
    superfluous/drop_non_null_keywords_draft4.h
    superfluous/drop_non_null_keywords_draft6.h
    superfluous/drop_non_null_keywords_draft7.h
    superfluous/drop_non_null_keywords_format_2019_09.h
    superfluous/drop_non_null_keywords_format_2020_12.h
    superfluous/drop_non_null_keywords_unevaluated_2020_12.h
    superfluous/drop_non_null_keywords_validation_2019_09.h
    superfluous/drop_non_null_keywords_validation_2020_12.h
    superfluous/drop_non_numeric_keywords_applicator_2019_09.h
    superfluous/drop_non_numeric_keywords_applicator_2020_12.h
    superfluous/drop_non_numeric_keywords_content_2019_09.h
    superfluous/drop_non_numeric_keywords_content_2020_12.h
    superfluous/drop_non_numeric_keywords_draft0.h
    superfluous/drop_non_numeric_keywords_draft1.h
    superfluous/drop_non_numeric_keywords_draft2.h
    superfluous/drop_non_numeric_keywords_draft3.h
    superfluous/drop_non_numeric_keywords_draft4.h
    superfluous/drop_non_numeric_keywords_draft6.h
    superfluous/drop_non_numeric_keywords_draft7.h
    superfluous/drop_non_numeric_keywords_format_2019_09.h
    superfluous/drop_non_numeric_keywords_format_2020_12.h
    superfluous/drop_non_numeric_keywords_unevaluated_2020_12.h
    superfluous/drop_non_numeric_keywords_validation_2019_09.h
    superfluous/drop_non_numeric_keywords_validation_2020_12.h
    superfluous/drop_non_object_keywords_applicator_2019_09.h
    superfluous/drop_non_object_keywords_applicator_2020_12.h
    superfluous/drop_non_object_keywords_content_2019_09.h
    superfluous/drop_non_object_keywords_content_2020_12.h
    superfluous/drop_non_object_keywords_draft0.h
    superfluous/drop_non_object_keywords_draft1.h
    superfluous/drop_non_object_keywords_draft2.h
    superfluous/drop_non_object_keywords_draft3.h
    superfluous/drop_non_object_keywords_draft4.h
    superfluous/drop_non_object_keywords_draft6.h
    superfluous/drop_non_object_keywords_draft7.h
    superfluous/drop_non_object_keywords_format_2019_09.h
    superfluous/drop_non_object_keywords_format_2020_12.h
    superfluous/drop_non_object_keywords_unevaluated_2020_12.h
    superfluous/drop_non_object_keywords_validation_2019_09.h
    superfluous/drop_non_object_keywords_validation_2020_12.h
    superfluous/drop_non_string_keywords_applicator_2019_09.h
    superfluous/drop_non_string_keywords_applicator_2020_12.h
    superfluous/drop_non_string_keywords_draft0.h
    superfluous/drop_non_string_keywords_draft1.h
    superfluous/drop_non_string_keywords_draft2.h
    superfluous/drop_non_string_keywords_draft3.h
    superfluous/drop_non_string_keywords_draft4.h
    superfluous/drop_non_string_keywords_draft6.h
    superfluous/drop_non_string_keywords_draft7.h
    superfluous/drop_non_string_keywords_unevaluated_2020_12.h
    superfluous/drop_non_string_keywords_validation_2019_09.h
    superfluous/drop_non_string_keywords_validation_2020_12.h
    superfluous/duplicate_allof_branches.h
    superfluous/duplicate_anyof_branches.h
    superfluous/else_without_if.h
    superfluous/if_without_then_else.h
    superfluous/max_contains_without_contains.h
    superfluous/min_contains_without_contains.h
    superfluous/then_without_if.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME alterschema)
endif()

target_link_libraries(sourcemeta_core_alterschema PUBLIC
  sourcemeta::core::jsonschema)
