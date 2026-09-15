#include <sourcemeta/blaze/convert.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <algorithm> // std::ranges::any_of
#include <array>     // std::array
#include <cassert>   // assert
#include <concepts>  // std::derived_from
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint64_t
#include <functional> // std::cref, std::function, std::hash, std::reference_wrapper
#include <map>        // std::map
#include <memory>     // std::make_unique, std::unique_ptr
#include <optional>      // std::optional, std::nullopt
#include <set>           // std::set
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <type_traits>   // std::is_same_v, std::true_type, std::false_type
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::pair
#include <vector>        // std::vector

namespace sourcemeta::blaze {

using namespace sourcemeta::core;

namespace {

#include "rule.h"

using Rule = std::tuple<std::unique_ptr<SchemaTransformRule>, bool>;

/// Construct a rule entry for the given rule type
template <std::derived_from<SchemaTransformRule> T>
[[nodiscard]] auto make_rule() -> Rule {
  return {std::make_unique<T>(),
          std::is_same_v<typename T::reframe_after_transform, std::true_type>};
}

/// Apply the given rules top-down to every subschema until none of them applies
auto apply(const std::vector<Rule> &rules, sourcemeta::core::JSON &schema,
           const sourcemeta::core::SchemaWalker &walker,
           const sourcemeta::core::SchemaResolver &resolver,
           const std::string_view default_dialect,
           const std::string_view default_id, const bool is_metaschema)
    -> void {
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

  std::optional<core::SchemaFrame> frame;

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
    if (!frame.has_value()) {
      if (schema.is_boolean()) {
        break;
      }

      frame.emplace(core::SchemaFrame::Mode::References, schema, walker,
                    resolver, default_dialect, default_id,
                    sourcemeta::core::SchemaFrame::IdentifierMode::Fallback);
    }

    std::unordered_set<core::Pointer, core::Pointer::Hasher> visited;
    bool applied{false};

    // Stopping the traversal stands in for the restart that the
    // rules request once they mutate the schema
    [[maybe_unused]] const auto restarted{frame->any_subschema(
        [&](const core::SchemaFrame::Location &location) -> bool {
          const auto [visited_iterator, inserted] =
              visited.insert(core::to_pointer(location.pointer));
          if (!inserted) {
            return false;
          }
          const auto &entry_pointer{*visited_iterator};
          auto &current{core::get(schema, entry_pointer)};
          const auto current_vocabularies{
              frame->vocabularies(location, resolver)};

          for (const auto &[rule, reframe_after_transform] : rules) {
            const auto outcome{
                rule->condition(current, schema, current_vocabularies, *frame,
                                location, walker, resolver, is_metaschema)};

            if (!outcome) {
              continue;
            }

            potentially_broken_references.clear();
            frame->for_each_reference([&](const core::SchemaReferenceType,
                                          const core::WeakPointer &origin,
                                          const core::SchemaFrame::Reference
                                              &reference) -> void {
              const auto destination{frame->traverse(reference.destination)};
              if (!destination.has_value() || !reference.fragment.has_value() ||
                  !reference.fragment.value().starts_with('/')) {
                return;
              }

              const auto &target{destination.value().get()};
              potentially_broken_references.push_back(
                  {.origin = core::to_pointer(origin),
                   .original = core::JSON::String{reference.original},
                   .destination = reference.destination,
                   .fragment = core::JSON::String{reference.fragment.value()},
                   .target_pointer = core::to_pointer(target.pointer),
                   .target_relative_pointer = target.relative_pointer});
            });

            rule->transform(current);

            applied = true;

            if (reframe_after_transform) {
              frame.emplace(
                  core::SchemaFrame::Mode::References, schema, walker, resolver,
                  default_dialect, default_id,
                  sourcemeta::core::SchemaFrame::IdentifierMode::Fallback);
            } else if (current.is_boolean()) {
              std::tuple<core::Pointer, std::string_view, core::JSON> mark{
                  entry_pointer, rule->name(), current};
              assert(!processed_rules.contains(mark));
              processed_rules.emplace(std::move(mark));
              frame.reset();
              return true;
            }

            const auto new_location{
                frame->traverse(core::to_weak_pointer(entry_pointer))};
            assert(new_location.has_value());

            // Fix broken references before re-checking the condition,
            // as the re-check may mutate rule state that rereference needs
            bool references_fixed{false};
            const auto resource_offset{
                new_location.value().get().relative_pointer};
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
                    saved_reference.origin.slice(resource_offset),
                    current_slice)};
                if (!new_origin.has_value()) {
                  continue;
                }
                effective_origin =
                    saved_reference.origin.slice(0, resource_offset)
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
                throw ConvertBrokenReferenceError{saved_reference.destination,
                                                  saved_reference.origin};
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
              core::set(schema, effective_origin,
                        core::JSON{original.recompose()});
              references_fixed = true;
            }

            const auto new_vocabularies{
                frame->vocabularies(new_location.value().get(), resolver)};

            assert(!rule->condition(current, schema, new_vocabularies, *frame,
                                    new_location.value().get(), walker,
                                    resolver, is_metaschema));

            std::tuple<core::Pointer, std::string_view, core::JSON> mark{
                entry_pointer, rule->name(), current};
            assert(!processed_rules.contains(mark));
            processed_rules.emplace(std::move(mark));

            if (references_fixed) {
              frame.reset();
            }

            if (references_fixed || reframe_after_transform) {
              return true;
            }
          }

          return false;
        })};

    if (!applied) {
      break;
    }
  }
}

#include "helpers.h"

#include "rules/definitions_to_defs.h"
#include "rules/draft_official_dialect_with_https.h"
#include "rules/draft_official_dialect_without_empty_fragment.h"
#include "rules/empty_object_as_true.h"
#include "rules/enum_to_const.h"
#include "rules/prefix_promoted_2020_12_keywords.h"
#include "rules/prefix_promoted_draft_2019_09_keywords.h"
#include "rules/prefix_promoted_draft_4_keywords.h"
#include "rules/prefix_promoted_draft_6_keywords.h"
#include "rules/prefix_promoted_draft_7_keywords.h"
#include "rules/upgrade_2019_09_to_2020_12.h"
#include "rules/upgrade_dialect_override_cleanup.h"
#include "rules/upgrade_draft_3_to_draft_4.h"
#include "rules/upgrade_draft_4_to_draft_6.h"
#include "rules/upgrade_draft_6_to_draft_7.h"
#include "rules/upgrade_draft_7_to_draft_2019_09.h"

#undef ONLY_CONTINUE_IF

} // namespace

auto convert(sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const ConvertTarget target, const std::string_view default_dialect,
             const std::string_view default_id, const bool is_metaschema)
    -> void {
  std::vector<Rule> rules;
  rules.reserve(17);
  rules.push_back(make_rule<DraftOfficialDialectWithHttps>());
  rules.push_back(make_rule<DraftOfficialDialectWithoutEmptyFragment>());
  rules.push_back(make_rule<PrefixPromotedDraft4Keywords>());
  rules.push_back(make_rule<UpgradeDraft3ToDraft4>());

  if (target == ConvertTarget::Draft6 || target == ConvertTarget::Draft7 ||
      target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012) {
    rules.push_back(make_rule<PrefixPromotedDraft6Keywords>());
    rules.push_back(make_rule<UpgradeDraft4ToDraft6>());
    rules.push_back(make_rule<EmptyObjectAsTrue>());
  }

  if (target == ConvertTarget::Draft7 || target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012) {
    rules.push_back(make_rule<PrefixPromotedDraft7Keywords>());
    rules.push_back(make_rule<UpgradeDraft6ToDraft7>());
    rules.push_back(make_rule<EnumToConst>());
  }

  if (target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012) {
    rules.push_back(make_rule<PrefixPromoted201909Keywords>());
    rules.push_back(make_rule<UpgradeDraft7To201909>());
    rules.push_back(make_rule<DefinitionsToDefs>());
  }

  if (target == ConvertTarget::Draft202012) {
    rules.push_back(make_rule<PrefixPromoted202012Keywords>());
    rules.push_back(make_rule<Upgrade201909To202012>());
  }

  rules.push_back(make_rule<UpgradeDialectOverrideCleanup>());
  apply(rules, schema, walker, resolver, default_dialect, default_id,
        is_metaschema);
}

} // namespace sourcemeta::blaze
