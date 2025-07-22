#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" lint --list > "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
additional_items_with_schema_items
  The `additionalItems` keyword is ignored when the `items` keyword is set to a schema

additional_properties_default
  Setting the `additionalProperties` keyword to the true schema does not add any further constraint

blaze/valid_default
  Only set a `default` value that validates against the schema

blaze/valid_examples
  Only include instances in the `examples` array that validate against the schema

const_with_type
  Setting `type` alongside `const` is considered an anti-pattern, as the constant already implies its respective type

content_media_type_without_encoding
  The `contentMediaType` keyword is meaningless without the presence of the `contentEncoding` keyword

content_schema_default
  Setting the `contentSchema` keyword to the true schema does not add any further constraint

content_schema_without_media_type
  The `contentSchema` keyword is meaningless without the presence of the `contentMediaType` keyword

dependencies_default
  Setting the `dependencies` keyword to an empty object does not add any further constraint

dependencies_property_tautology
  Defining requirements for a property using `dependencies` that is already marked as required is an unnecessarily complex use of `dependencies`

dependent_required_default
  Setting the `dependentRequired` keyword to an empty object does not add any further constraint

dependent_required_tautology
  Defining requirements for a property using `dependentRequired` that is already marked as required is an unnecessarily complex use of `dependentRequired`

draft_official_dialect_without_empty_fragment
  The official dialect URI of Draft 7 and older versions must contain the empty fragment

duplicate_allof_branches
  Setting duplicate subschemas in `allOf` is redundant, as it produces unnecessary additional validation that is guaranteed to not affect the validation result

duplicate_anyof_branches
  Setting duplicate subschemas in `anyOf` is redundant, as it produces unnecessary additional validation that is guaranteed to not affect the validation result

duplicate_enum_values
  Setting duplicate values in `enum` is considered an anti-pattern

duplicate_required_values
  Setting duplicate values in `required` is considered an anti-pattern

else_empty
  Setting the `else` keyword to the empty schema does not add any further constraint

else_without_if
  The `else` keyword is meaningless without the presence of the `if` keyword

enum_to_const
  An `enum` of a single value can be expressed as `const`

enum_with_type
  Setting `type` alongside `enum` is considered an anti-pattern, as the enumeration choices already imply their respective types

equal_numeric_bounds_to_enum
  Setting `minimum` and `maximum` to the same number only leaves one possible value

exclusive_maximum_number_and_maximum
  Setting both `exclusiveMaximum` and `maximum` at the same time is considered an anti-pattern. You should choose one

exclusive_minimum_number_and_minimum
  Setting both `exclusiveMinimum` and `minimum` at the same time is considered an anti-pattern. You should choose one

if_without_then_else
  The `if` keyword is meaningless without the presence of the `then` or `else` keywords

items_array_default
  Setting the `items` keyword to the empty array does not add any further constraint

items_schema_default
  Setting the `items` keyword to the true schema does not add any further constraint

max_contains_without_contains
  The `maxContains` keyword is meaningless without the presence of the `contains` keyword

maximum_real_for_integer
  If an instance is guaranteed to be an integer, setting a real number upper bound is the same as a floor of that upper bound

min_contains_without_contains
  The `minContains` keyword is meaningless without the presence of the `contains` keyword

minimum_real_for_integer
  If an instance is guaranteed to be an integer, setting a real number lower bound is the same as a ceil of that lower bound

modern_official_dialect_with_empty_fragment
  The official dialect URI of 2019-09 and newer versions must not contain the empty fragment

multiple_of_default
  Setting `multipleOf` to 1 does not add any further constraint

non_applicable_type_specific_keywords
  Avoid keywords that don't apply to the current explicitly declared type

pattern_properties_default
  Setting the `patternProperties` keyword to the empty object does not add any further constraint

properties_default
  Setting the `properties` keyword to the empty object does not add any further constraint

single_type_array
  Setting `type` to an array of a single type is the same as directly declaring such type

then_empty
  Setting the `then` keyword to the empty schema does not add any further constraint

then_without_if
  The `then` keyword is meaningless without the presence of the `if` keyword

unevaluated_items_default
  Setting the `unevaluatedItems` keyword to the true schema does not add any further constraint

unevaluated_properties_default
  Setting the `unevaluatedProperties` keyword to the true schema does not add any further constraint

unnecessary_allof_wrapper_draft
  Wrapping any keyword other than `$ref` in `allOf` is unnecessary

unnecessary_allof_wrapper_modern
  Wrapping any keyword in `allOf` is unnecessary

unnecessary_allof_wrapper_properties
  Avoid unnecessarily wrapping object `properties` in `allOf`

unsatisfiable_max_contains
  Setting the `maxContains` keyword to a number greater than or equal to the array upper bound does not add any further constraint

unsatisfiable_min_properties
  Setting `minProperties` to a number less than `required` does not add any further constraint

Number of rules: 46
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
