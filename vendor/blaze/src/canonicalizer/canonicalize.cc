#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>     // std::sort, std::unique, std::ranges::none_of
#include <array>         // std::array
#include <bit>           // std::popcount
#include <cassert>       // assert
#include <cmath>         // std::floor, std::ceil, std::isfinite
#include <concepts>      // std::derived_from
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint64_t
#include <functional>    // std::hash, std::ref
#include <limits>        // std::numeric_limits
#include <memory>        // std::make_unique, std::unique_ptr
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <type_traits>   // std::is_same_v, std::true_type
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::to_underlying
#include <vector>        // std::vector

namespace sourcemeta::blaze {

namespace {

#include "rule.h"

using Rule = std::tuple<std::unique_ptr<SchemaTransformRule>, bool>;

/// Construct a rule entry for the given rule type
template <std::derived_from<SchemaTransformRule> T>
[[nodiscard]] auto make_rule() -> Rule {
  return {std::make_unique<T>(),
          std::is_same_v<typename T::reframe_after_transform, std::true_type>};
}

/// Re-analyse the frame, honouring any identifier already present in the schema
auto analyse_frame(sourcemeta::blaze::SchemaFrame &target,
                   sourcemeta::core::JSON &schema,
                   const sourcemeta::blaze::SchemaWalker &walker,
                   const sourcemeta::blaze::SchemaResolver &resolver,
                   const std::string_view default_dialect,
                   const std::string_view default_id) -> void {
  if (!blaze::identify(schema, resolver, default_dialect).empty()) {
    target.analyse(schema, walker, resolver, default_dialect);
  } else {
    target.analyse(schema, walker, resolver, default_dialect, default_id);
  }
}

/// Apply the given rules top-down to every subschema until none of them applies
auto apply(const std::vector<Rule> &rules, sourcemeta::core::JSON &schema,
           const sourcemeta::blaze::SchemaWalker &walker,
           const sourcemeta::blaze::SchemaResolver &resolver,
           const std::string_view default_dialect = "",
           const std::string_view default_id = "") -> void {
  assert(!rules.empty());

  struct ProcessedRuleHasher {
    auto operator()(const std::tuple<core::Pointer, std::string_view,
                                     core::JSON> &value) const noexcept
        -> std::size_t {
      return core::Pointer::Hasher{}(std::get<0>(value)) ^
             (std::hash<std::string_view>{}(std::get<1>(value)) << 1) ^
             (std::hash<std::uint64_t>{}(std::get<2>(value).fast_hash()) << 2);
    }
  };

  std::unordered_set<std::tuple<core::Pointer, std::string_view, core::JSON>,
                     ProcessedRuleHasher>
      processed_rules;

  blaze::SchemaFrame frame{blaze::SchemaFrame::Mode::References};

  struct PotentiallyBrokenReference {
    core::Pointer origin;
    core::JSON::String original;
    core::JSON::String destination;
    core::JSON::String fragment;
    core::Pointer target_pointer;
    std::size_t target_relative_pointer;
  };

  std::vector<PotentiallyBrokenReference> potentially_broken_references;

  while (true) {
    if (frame.empty()) {
      if (schema.is_boolean()) {
        break;
      }

      analyse_frame(frame, schema, walker, resolver, default_dialect,
                    default_id);
    }

    std::unordered_set<core::Pointer, core::Pointer::Hasher> visited;
    bool applied{false};

    for (const auto &entry : frame.locations()) {
      if (entry.second.type != blaze::SchemaFrame::LocationType::Resource &&
          entry.second.type != blaze::SchemaFrame::LocationType::Subschema) {
        continue;
      }

      const auto [visited_iterator, inserted] =
          visited.insert(core::to_pointer(entry.second.pointer));
      if (!inserted) {
        continue;
      }
      const auto &entry_pointer{*visited_iterator};
      auto &current{core::get(schema, entry_pointer)};
      const auto current_vocabularies{
          frame.vocabularies(entry.second, resolver)};

      for (const auto &[rule, reframe_after_transform] : rules) {
        const auto outcome{rule->condition(current, schema,
                                           current_vocabularies, frame,
                                           entry.second, walker, resolver)};

        if (!outcome) {
          continue;
        }

        potentially_broken_references.clear();
        for (const auto &reference : frame.references()) {
          const auto destination{frame.traverse(reference.second.destination)};
          if (!destination.has_value() ||
              !reference.second.fragment.has_value() ||
              !reference.second.fragment.value().starts_with('/')) {
            continue;
          }

          const auto &target{destination.value().get()};
          potentially_broken_references.push_back(
              {.origin = core::to_pointer(reference.first.second),
               .original = core::JSON::String{reference.second.original},
               .destination = reference.second.destination,
               .fragment =
                   core::JSON::String{reference.second.fragment.value()},
               .target_pointer = core::to_pointer(target.pointer),
               .target_relative_pointer = target.relative_pointer});
        }

        rule->transform(current);

        applied = true;

        if (reframe_after_transform) {
          analyse_frame(frame, schema, walker, resolver, default_dialect,
                        default_id);
        } else if (current.is_boolean()) {
          std::tuple<core::Pointer, std::string_view, core::JSON> mark{
              entry_pointer, rule->name(), current};
          assert(!processed_rules.contains(mark));
          processed_rules.emplace(std::move(mark));
          frame.reset();
          goto blaze_transformer_start_again;
        }

        const auto new_location{
            frame.traverse(core::to_weak_pointer(entry_pointer))};
        assert(new_location.has_value());

        // Fix broken references before re-checking the condition,
        // as the re-check may mutate rule state that rereference needs
        bool references_fixed{false};
        const auto resource_offset{new_location.value().get().relative_pointer};
        const auto current_slice{entry_pointer.slice(resource_offset)};
        for (const auto &saved_reference : potentially_broken_references) {
          if (core::try_get(schema, saved_reference.target_pointer)) {
            continue;
          }

          // If the origin was also relocated, resolve its new location
          auto effective_origin{saved_reference.origin};
          if (!core::try_get(schema, saved_reference.origin.initial())) {
            const auto new_origin{rule->rereference(
                saved_reference.destination, saved_reference.origin,
                saved_reference.origin.slice(resource_offset), current_slice)};
            if (!new_origin.has_value()) {
              continue;
            }
            effective_origin = saved_reference.origin.slice(0, resource_offset)
                                   .concat(new_origin.value());
            if (!core::try_get(schema, effective_origin.initial())) {
              continue;
            }
          }

          const auto new_relative{rule->rereference(
              saved_reference.destination, saved_reference.origin,
              saved_reference.target_pointer.slice(
                  saved_reference.target_relative_pointer),
              current_slice)};
          if (!new_relative.has_value()) {
            continue;
          }
          const auto new_fragment{
              saved_reference.fragment ==
                      core::to_string(saved_reference.target_pointer)
                  ? saved_reference.target_pointer
                        .slice(0, saved_reference.target_relative_pointer)
                        .concat(new_relative.value())
                  : new_relative.value()};

          core::URI original{saved_reference.original};
          original.fragment(core::to_string(new_fragment));
          core::set(schema, effective_origin, core::JSON{original.recompose()});
          references_fixed = true;
        }

        const auto new_vocabularies{
            frame.vocabularies(new_location.value().get(), resolver)};

        assert(!rule->condition(current, schema, new_vocabularies, frame,
                                new_location.value().get(), walker, resolver));

        std::tuple<core::Pointer, std::string_view, core::JSON> mark{
            entry_pointer, rule->name(), current};
        assert(!processed_rules.contains(mark));
        processed_rules.emplace(std::move(mark));

        if (references_fixed) {
          frame.reset();
        }

        if (references_fixed || reframe_after_transform) {
          goto blaze_transformer_start_again;
        }
      }
    }

  blaze_transformer_start_again:
    if (!applied) {
      break;
    }
  }
}

#include "helpers.h"

#include "rules/additional_items_implicit.h"
#include "rules/allof_false_simplify.h"
#include "rules/allof_merge_compatible_branches.h"
#include "rules/anyof_false_simplify.h"
#include "rules/anyof_remove_false_schemas.h"
#include "rules/anyof_true_simplify.h"
#include "rules/comment_drop.h"
#include "rules/const_as_enum.h"
#include "rules/const_in_enum.h"
#include "rules/const_with_type.h"
#include "rules/content_media_type_without_encoding.h"
#include "rules/content_schema_without_media_type.h"
#include "rules/definitions_to_defs.h"
#include "rules/dependencies_property_tautology.h"
#include "rules/dependencies_to_any_of.h"
#include "rules/dependencies_to_extends_disallow.h"
#include "rules/dependent_required_tautology.h"
#include "rules/dependent_required_to_any_of.h"
#include "rules/dependent_schemas_to_any_of.h"
#include "rules/deprecated_false_drop.h"
#include "rules/disallow_array_to_extends.h"
#include "rules/disallow_double_negation.h"
#include "rules/disallow_extends_to_type.h"
#include "rules/disallow_narrows_type.h"
#include "rules/disallow_to_array_of_schemas.h"
#include "rules/disallow_type_union_to_extends.h"
#include "rules/divisible_by_implicit.h"
#include "rules/double_negation_elimination.h"
#include "rules/draft3_type_any.h"
#include "rules/draft_official_dialect_with_https.h"
#include "rules/draft_official_dialect_without_empty_fragment.h"
#include "rules/draft_ref_siblings.h"
#include "rules/drop_allof_empty_schemas.h"
#include "rules/drop_extends_empty_schemas.h"
#include "rules/duplicate_allof_branches.h"
#include "rules/duplicate_anyof_branches.h"
#include "rules/duplicate_disallow_entries.h"
#include "rules/duplicate_enum_values.h"
#include "rules/duplicate_required_values.h"
#include "rules/dynamic_ref_to_static_ref.h"
#include "rules/else_without_if.h"
#include "rules/empty_definitions_drop.h"
#include "rules/empty_defs_drop.h"
#include "rules/empty_dependencies_drop.h"
#include "rules/empty_dependent_required_drop.h"
#include "rules/empty_dependent_schemas_drop.h"
#include "rules/empty_disallow_drop.h"
#include "rules/empty_object_as_true.h"
#include "rules/enum_drop_redundant_validation.h"
#include "rules/enum_filter_by_type.h"
#include "rules/enum_split_by_type.h"
#include "rules/enum_with_type.h"
#include "rules/equal_numeric_bounds_to_const.h"
#include "rules/equal_numeric_bounds_to_enum.h"
#include "rules/exclusive_bounds_false_drop.h"
#include "rules/exclusive_maximum_boolean_integer_fold.h"
#include "rules/exclusive_maximum_integer_to_maximum.h"
#include "rules/exclusive_maximum_number_and_maximum.h"
#include "rules/exclusive_minimum_boolean_integer_fold.h"
#include "rules/exclusive_minimum_integer_to_minimum.h"
#include "rules/exclusive_minimum_number_and_minimum.h"
#include "rules/extends_to_array.h"
#include "rules/flatten_nested_allof.h"
#include "rules/flatten_nested_anyof.h"
#include "rules/flatten_nested_extends.h"
#include "rules/if_then_else_implicit.h"
#include "rules/if_without_then_else.h"
#include "rules/ignored_metaschema.h"
#include "rules/implicit_contains_keywords.h"
#include "rules/implicit_object_keywords.h"
#include "rules/inline_single_use_ref.h"
#include "rules/items_implicit.h"
#include "rules/max_contains_covered_by_max_items.h"
#include "rules/max_contains_without_contains.h"
#include "rules/max_decimal_implicit.h"
#include "rules/maximum_can_equal_integer_fold.h"
#include "rules/maximum_can_equal_true_drop.h"
#include "rules/maximum_real_for_integer.h"
#include "rules/min_contains_without_contains.h"
#include "rules/min_items_given_min_contains.h"
#include "rules/min_length_implicit.h"
#include "rules/min_properties_covered_by_required.h"
#include "rules/minimum_can_equal_integer_fold.h"
#include "rules/minimum_can_equal_true_drop.h"
#include "rules/minimum_real_for_integer.h"
#include "rules/modern_official_dialect_with_empty_fragment.h"
#include "rules/modern_official_dialect_with_http.h"
#include "rules/multiple_of_implicit.h"
#include "rules/non_applicable_additional_items.h"
#include "rules/non_applicable_disallow_types.h"
#include "rules/non_applicable_enum_validation_keywords.h"
#include "rules/non_applicable_type_specific_keywords.h"
#include "rules/not_false.h"
#include "rules/oneof_false_simplify.h"
#include "rules/oneof_to_anyof_disjoint_types.h"
#include "rules/optional_property_implicit.h"
#include "rules/orphan_definitions.h"
#include "rules/recursive_anchor_false_drop.h"
#include "rules/required_properties_in_properties.h"
#include "rules/required_property_implicit.h"
#include "rules/required_to_extends.h"
#include "rules/single_branch_allof.h"
#include "rules/single_branch_anyof.h"
#include "rules/single_branch_oneof.h"
#include "rules/single_type_array.h"
#include "rules/then_without_if.h"
#include "rules/type_array_to_any_of.h"
#include "rules/type_boolean_as_enum.h"
#include "rules/type_inherit_in_place.h"
#include "rules/type_null_as_enum.h"
#include "rules/type_union_distribute_keywords.h"
#include "rules/type_union_implicit.h"
#include "rules/type_union_to_schemas.h"
#include "rules/type_with_applicator_to_allof.h"
#include "rules/type_with_applicator_to_extends.h"
#include "rules/unevaluated_items_to_items.h"
#include "rules/unevaluated_properties_to_additional_properties.h"
#include "rules/unknown_keywords_prefix.h"
#include "rules/unknown_local_ref.h"
#include "rules/unnecessary_allof_ref_wrapper_draft.h"
#include "rules/unnecessary_extends_ref_wrapper.h"
#include "rules/unsatisfiable_drop_validation.h"
#include "rules/unsatisfiable_exclusive_equal_bounds.h"
#include "rules/unsatisfiable_in_place_applicator_type.h"
#include "rules/unsatisfiable_type_and_enum.h"

#undef ONLY_CONTINUE_IF

} // namespace

auto canonicalize(sourcemeta::core::JSON &schema,
                  const sourcemeta::blaze::SchemaWalker &walker,
                  const sourcemeta::blaze::SchemaResolver &resolver,
                  const std::string_view default_dialect,
                  const std::string_view default_id) -> void {
  std::vector<Rule> rules;
  rules.reserve(125);
  rules.push_back(make_rule<ExclusiveMinimumBooleanIntegerFold>());
  rules.push_back(make_rule<ExclusiveMaximumBooleanIntegerFold>());
  rules.push_back(make_rule<UnsatisfiableExclusiveEqualBounds>());
  rules.push_back(make_rule<MinimumCanEqualIntegerFold>());
  rules.push_back(make_rule<MaximumCanEqualIntegerFold>());
  rules.push_back(make_rule<MinimumCanEqualTrueDrop>());
  rules.push_back(make_rule<MaximumCanEqualTrueDrop>());
  rules.push_back(make_rule<CommentDrop>());
  rules.push_back(make_rule<DeprecatedFalseDrop>());
  rules.push_back(make_rule<RecursiveAnchorFalseDrop>());
  rules.push_back(make_rule<UnevaluatedItemsToItems>());
  rules.push_back(make_rule<UnevaluatedPropertiesToAdditionalProperties>());
  rules.push_back(make_rule<IfThenElseImplicit>());
  rules.push_back(make_rule<ImplicitObjectKeywords>());
  rules.push_back(make_rule<ImplicitContainsKeywords>());
  rules.push_back(make_rule<ExtendsToArray>());
  rules.push_back(make_rule<DisallowToArrayOfSchemas>());
  rules.push_back(make_rule<InlineSingleUseRef>());
  rules.push_back(make_rule<AllOfMergeCompatibleBranches>());
  rules.push_back(make_rule<TypeInheritInPlace>());
  rules.push_back(make_rule<TypeUnionImplicit>());
  rules.push_back(make_rule<TypeArrayToAnyOf>());
  rules.push_back(make_rule<DefinitionsToDefs>());
  rules.push_back(make_rule<ContentMediaTypeWithoutEncoding>());
  rules.push_back(make_rule<ContentSchemaWithoutMediaType>());
  rules.push_back(make_rule<DraftOfficialDialectWithHttps>());
  rules.push_back(make_rule<DraftOfficialDialectWithoutEmptyFragment>());
  rules.push_back(make_rule<NonApplicableTypeSpecificKeywords>());
  rules.push_back(make_rule<NonApplicableDisallowTypes>());
  rules.push_back(make_rule<DisallowNarrowsType>());
  rules.push_back(make_rule<AnyOfRemoveFalseSchemas>());
  rules.push_back(make_rule<AnyOfTrueSimplify>());
  rules.push_back(make_rule<DuplicateAllOfBranches>());
  rules.push_back(make_rule<DuplicateAnyOfBranches>());
  rules.push_back(make_rule<FlattenNestedAllOf>());
  rules.push_back(make_rule<FlattenNestedExtends>());
  rules.push_back(make_rule<FlattenNestedAnyOf>());
  rules.push_back(make_rule<Draft3TypeAny>());
  rules.push_back(make_rule<UnsatisfiableInPlaceApplicatorType>());
  rules.push_back(make_rule<AllOfFalseSimplify>());
  rules.push_back(make_rule<AnyOfFalseSimplify>());
  rules.push_back(make_rule<OneOfFalseSimplify>());
  rules.push_back(make_rule<DoubleNegationElimination>());
  rules.push_back(make_rule<OneOfToAnyOfDisjointTypes>());
  rules.push_back(make_rule<UnsatisfiableDropValidation>());
  rules.push_back(make_rule<ElseWithoutIf>());
  rules.push_back(make_rule<IfWithoutThenElse>());
  rules.push_back(make_rule<IgnoredMetaschema>());
  rules.push_back(make_rule<MaxContainsWithoutContains>());
  rules.push_back(make_rule<MinContainsWithoutContains>());
  rules.push_back(make_rule<NotFalse>());
  rules.push_back(make_rule<ThenWithoutIf>());
  rules.push_back(make_rule<DependenciesPropertyTautology>());
  rules.push_back(make_rule<DependentRequiredTautology>());
  rules.push_back(make_rule<EqualNumericBoundsToEnum>());
  rules.push_back(make_rule<MaximumRealForInteger>());
  rules.push_back(make_rule<MinimumRealForInteger>());
  rules.push_back(make_rule<SingleTypeArray>());
  rules.push_back(make_rule<EnumWithType>());
  rules.push_back(make_rule<NonApplicableEnumValidationKeywords>());
  rules.push_back(make_rule<DuplicateEnumValues>());
  rules.push_back(make_rule<DuplicateRequiredValues>());
  rules.push_back(make_rule<ConstWithType>());
  rules.push_back(make_rule<ConstInEnum>());
  rules.push_back(make_rule<NonApplicableAdditionalItems>());
  rules.push_back(make_rule<ModernOfficialDialectWithEmptyFragment>());
  rules.push_back(make_rule<ModernOfficialDialectWithHttp>());
  rules.push_back(make_rule<ExclusiveMaximumNumberAndMaximum>());
  rules.push_back(make_rule<ExclusiveMinimumNumberAndMinimum>());
  rules.push_back(make_rule<ExclusiveBoundsFalseDrop>());
  rules.push_back(make_rule<DraftRefSiblings>());
  rules.push_back(make_rule<DynamicRefToStaticRef>());
  rules.push_back(make_rule<UnknownKeywordsPrefix>());
  rules.push_back(make_rule<UnknownLocalRef>());
  rules.push_back(make_rule<RequiredPropertiesInProperties>());
  rules.push_back(make_rule<OrphanDefinitions>());
  rules.push_back(make_rule<ConstAsEnum>());
  rules.push_back(make_rule<EqualNumericBoundsToConst>());
  rules.push_back(make_rule<ExclusiveMaximumIntegerToMaximum>());
  rules.push_back(make_rule<ExclusiveMinimumIntegerToMinimum>());
  rules.push_back(make_rule<TypeBooleanAsEnum>());
  rules.push_back(make_rule<TypeNullAsEnum>());
  rules.push_back(make_rule<MaxContainsCoveredByMaxItems>());
  rules.push_back(make_rule<MinItemsGivenMinContains>());
  rules.push_back(make_rule<MinPropertiesCoveredByRequired>());
  rules.push_back(make_rule<MinLengthImplicit>());
  rules.push_back(make_rule<MultipleOfImplicit>());
  rules.push_back(make_rule<DivisibleByImplicit>());
  rules.push_back(make_rule<MaxDecimalImplicit>());
  rules.push_back(make_rule<ItemsImplicit>());
  rules.push_back(make_rule<UnnecessaryAllOfRefWrapperDraft>());
  rules.push_back(make_rule<UnnecessaryExtendsRefWrapper>());
  rules.push_back(make_rule<DropAllOfEmptySchemas>());
  rules.push_back(make_rule<DropExtendsEmptySchemas>());
  rules.push_back(make_rule<EmptyObjectAsTrue>());
  rules.push_back(make_rule<UnsatisfiableTypeAndEnum>());
  rules.push_back(make_rule<EnumFilterByType>());
  rules.push_back(make_rule<TypeUnionToSchemas>());
  rules.push_back(make_rule<TypeUnionDistributeKeywords>());
  rules.push_back(make_rule<DependenciesToAnyOf>());
  rules.push_back(make_rule<DependenciesToExtendsDisallow>());
  rules.push_back(make_rule<DependentSchemasToAnyOf>());
  rules.push_back(make_rule<DependentRequiredToAnyOf>());
  rules.push_back(make_rule<EnumDropRedundantValidation>());
  rules.push_back(make_rule<EnumSplitByType>());
  rules.push_back(make_rule<TypeWithApplicatorToAllOf>());
  rules.push_back(make_rule<TypeWithApplicatorToExtends>());
  rules.push_back(make_rule<EmptyDefinitionsDrop>());
  rules.push_back(make_rule<EmptyDefsDrop>());
  rules.push_back(make_rule<EmptyDependenciesDrop>());
  rules.push_back(make_rule<EmptyDependentSchemasDrop>());
  rules.push_back(make_rule<EmptyDependentRequiredDrop>());
  rules.push_back(make_rule<EmptyDisallowDrop>());
  rules.push_back(make_rule<AdditionalItemsImplicit>());
  rules.push_back(make_rule<RequiredPropertyImplicit>());
  rules.push_back(make_rule<OptionalPropertyImplicit>());
  rules.push_back(make_rule<DuplicateDisallowEntries>());
  rules.push_back(make_rule<DisallowArrayToExtends>());
  rules.push_back(make_rule<DisallowExtendsToType>());
  rules.push_back(make_rule<DisallowTypeUnionToExtends>());
  rules.push_back(make_rule<DisallowDoubleNegation>());
  rules.push_back(make_rule<RequiredToExtends>());
  rules.push_back(make_rule<SingleBranchAllOf>());
  rules.push_back(make_rule<SingleBranchAnyOf>());
  rules.push_back(make_rule<SingleBranchOneOf>());
  apply(rules, schema, walker, resolver, default_dialect, default_id);
}

} // namespace sourcemeta::blaze
