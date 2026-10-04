#include <sourcemeta/blaze/convert.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <algorithm> // std::ranges::any_of, std::ranges::find
#include <array>     // std::array
#include <cassert>
#include <concepts> // std::derived_from
#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint64_t
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

#include "helpers.h"
#include "rule.h"

using Rule = std::tuple<std::unique_ptr<SchemaTransformRule>, bool>;

/// Construct a rule entry for the given rule type
template <std::derived_from<SchemaTransformRule> T>
[[nodiscard]] auto make_rule() -> Rule {
  return {std::make_unique<T>(),
          std::is_same_v<typename T::writes_outside_itself, std::true_type>};
}

/// A reference that lands on something other than a schema is not a reference
/// the conversion can carry across dialects, as the document never had one
auto assert_schema_references(const core::SchemaFrame &frame) -> void {
  frame.for_each_reference(
      [&frame](const core::SchemaReferenceType, const core::WeakPointer &origin,
               const core::SchemaFrame::Reference &reference) -> void {
        const auto destination{frame.traverse(reference.destination)};
        if (destination.has_value() &&
            destination.value().get().type ==
                core::SchemaFrame::LocationType::Pointer) {
          throw ConvertInvalidReferenceError{reference.destination,
                                             core::to_pointer(origin)};
        }
      });
}

/// Conversion renames keywords, while a meta-schema names those same keywords
/// as ordinary data that nothing renames alongside them. Until the two can be
/// told apart, a document that describes itself or that carries the
/// meta-schema something in it declares is refused, and so is any resource
/// read as a dialect the ladder does not name
auto assert_convertible_dialects(const core::JSON &schema,
                                 const core::SchemaFrame &frame,
                                 const std::string_view default_id) -> void {
  const auto document{frame.traverse(core::EMPTY_WEAK_POINTER)};
  if (document.has_value() &&
      describes_itself(schema, document.value().get().base_dialect,
                       default_id)) {
    throw ConvertUnsupportedMetaschemaError{schema.at("$schema").to_string(),
                                            core::EMPTY_POINTER};
  }

  // A bundled meta-schema is what the resources naming it are refused for, so
  // it is looked for across the whole document before any of them is reported.
  // Otherwise whichever the frame happened to reach first would decide, and a
  // document carrying its own meta-schema would be reported against the
  // resource using it rather than against the meta-schema it cannot move
  frame.for_each_subschema(
      [&schema, &frame](const core::SchemaFrame::Location &location) -> void {
        auto pointer{core::to_pointer(location.pointer)};
        const auto &subschema{core::get(schema, pointer)};
        if (!is_metaschema_target(subschema, frame, location.pointer)) {
          return;
        }

        // The meta-schema that cannot be moved is what the error names. The
        // dialect that meta-schema is itself written in is an official one
        // the conversion supports perfectly well, so naming that instead
        // would report the evidence rather than the reason
        const auto *identifier{subschema.try_at(
            core::schema_identifier_keyword(location.base_dialect))};
        throw ConvertUnsupportedMetaschemaError{
            identifier != nullptr && identifier->is_string()
                ? std::string_view{identifier->to_string()}
                : location.dialect,
            std::move(pointer)};
      });

  // A dialect the ladder does not name has no rules for moving a schema off
  // it, and this conversion does not go near one. Carrying it along to the
  // targets it happens to outrank would convert the document around it and
  // leave the author to work out which parts moved, so a custom dialect is
  // refused outright whatever the target is
  frame.for_each_subschema(
      [&schema](const core::SchemaFrame::Location &location) -> void {
        auto pointer{core::to_pointer(location.pointer)};

        // What a subschema says about itself counts even where framing does
        // not read it that way. The ladder's own marker is read ahead of
        // `$schema`, so a document naming a custom meta-schema and carrying a
        // marker beside it frames as whatever the marker says, and asking
        // framing alone would let it through to be rewritten and have the
        // marker cleaned away underneath it.
        //
        // Only where a resource begins, though. A `$schema` deeper inside one
        // declares nothing, which is what both Draft 7 core 7 and 2019-09
        // core 8.1.1 say, so refusing over it would turn a string the author
        // left behind into an unconvertible document
        const auto &subschema{core::get(schema, pointer)};
        if (subschema.is_object() &&
            location.pointer.size() == location.relative_pointer) {
          const auto *declared{subschema.try_at("$schema")};
          if (declared != nullptr && declared->is_string() &&
              !names_ladder_dialect(declared->to_string())) {
            throw ConvertUnsupportedDialectError{declared->to_string(),
                                                 std::move(pointer)};
          }
        }

        if (names_ladder_dialect(location.dialect)) {
          return;
        }

        throw ConvertUnsupportedDialectError{location.dialect,
                                             std::move(pointer)};
      });
}

/// A schema that declares `$vocabulary` is a meta-schema, which is what this
/// conversion asks of one. Extending an official meta-schema means referencing
/// a document that recurses with the keyword of the dialect it was written for,
/// and nothing this conversion renames in the extending schema can carry that
/// recursion to another dialect, so such a meta-schema is refused rather than
/// quietly stripped of the constraints it places on what it describes
auto assert_convertible_metaschema(const core::JSON &schema,
                                   const core::SchemaFrame &frame) -> void {
  if (!schema.is_object()) {
    return;
  }

  // Declaring `$vocabulary` is what makes a schema a meta-schema here, and a
  // meta-schema that names an official one anywhere other than in its own
  // `$schema` is refused wherever that reference sits. Where the reference
  // appears does not soften it: the conversion cannot know whether the author
  // meant the schemas it describes to move dialect alongside it, and silently
  // bumping the meta-schema while leaving the reference on the old dialect
  // would decide that for them
  const auto extends_official{frame.any_reference(
      [](const core::SchemaReferenceType, const core::WeakPointer &origin,
         const core::SchemaFrame::Reference &reference) -> bool {
        if (!origin.empty() && origin.back().is_property() &&
            origin.back().to_property() == "$schema") {
          return false;
        }

        return names_official_metaschema(reference.destination);
      })};
  if (!extends_official) {
    return;
  }

  // The meta-schema is what cannot be converted, so it is what the error names,
  // and it may sit inside the document rather than be the whole of it
  frame.for_each_subschema(
      [&schema](const core::SchemaFrame::Location &location) -> void {
        auto pointer{core::to_pointer(location.pointer)};
        const auto &subschema{core::get(schema, pointer)};
        if (subschema.is_object() && subschema.defines("$vocabulary")) {
          throw ConvertUnsupportedMetaschemaError{location.dialect,
                                                  std::move(pointer)};
        }
      });
}

/// Follows every reference whose destination stopped resolving to wherever the
/// pass moved it. A reference breaks only when its destination stops
/// resolving: a target sitting at a different position is not enough, since a
/// resource that moved as a whole keeps resolving the fragments its own
/// identifier is the base of.
template <typename Snapshot, typename Relocation>
auto repair_references(sourcemeta::core::JSON &schema,
                       const std::vector<Snapshot> &snapshots,
                       const std::vector<Relocation> &journal,
                       const sourcemeta::core::SchemaWalker &walker,
                       const sourcemeta::core::SchemaResolver &resolver,
                       const std::string_view default_dialect,
                       const std::string_view default_id) -> void {
  if (snapshots.empty() || journal.empty() || schema.is_boolean()) {
    return;
  }

  const core::SchemaFrame frame{
      core::SchemaFrame::Mode::References,
      schema,
      walker,
      resolver,
      default_dialect,
      default_id,
      sourcemeta::core::SchemaFrame::IdentifierMode::Fallback};

  for (const auto &snapshot : snapshots) {
    if (frame.traverse(snapshot.destination).has_value()) {
      continue;
    }

    // The resource the fragment is written relative to can move in the same
    // pass as the target does, so where that resource now begins is followed
    // too. Slicing the moved target by where it used to begin would count
    // tokens of the new position as part of the old prefix
    auto target{snapshot.target};
    auto origin{snapshot.origin};
    auto base{snapshot.target.slice(0, snapshot.target_offset)};
    for (const auto &[before, after] : journal) {
      target = target.rebase(before, after);
      origin = origin.rebase(before, after);
      base = base.rebase(before, after);
    }

    // A pass is free to drop the subschema a reference was written in, and a
    // reference that is no longer in the document has nothing left to point
    // anywhere, so there is nothing to repair rather than anything broken
    if (core::try_get(schema, origin.initial()) == nullptr) {
      continue;
    }

    // Nothing the pass recorded accounts for where the target went, so the
    // reference cannot be followed
    if (target == snapshot.target) {
      throw ConvertBrokenReferenceError{snapshot.destination, snapshot.origin};
    }

    const auto relative{target.slice(base.size())};
    const auto fragment{snapshot.fragment == core::to_string(snapshot.target)
                            ? base.concat(relative)
                            : relative};

    core::URI original{snapshot.original};
    // The stringified pointer is literal text, so a token that already reads
    // as an escape must be encoded rather than taken as one
    original.unescaped_fragment(core::to_string(fragment));
    core::set(schema, origin, core::JSON{original.recompose()});
  }
}

/// Each pass frames the document once, decides every edit the pass will make
/// from that one frame, applies them deepest position first, and only then
/// repairs the references that the edits moved. Nothing re-frames in the
/// middle of a pass.
///
/// Applying the deepest position first is what lets a rule stop asking whether
/// the subschemas below it are still waiting to be rewritten: by the time a
/// position is reached, everything under it has already been handled in this
/// same pass.
auto apply(const std::vector<Rule> &rules, sourcemeta::core::JSON &schema,
           const sourcemeta::core::SchemaWalker &walker,
           const sourcemeta::core::SchemaResolver &resolver,
           const std::string_view default_dialect,
           const std::string_view default_id, const bool assert_convertible)
    -> void {
  assert(!rules.empty());

  struct Scheduled {
    Site site;
    core::SchemaVocabularies vocabularies;
    std::size_t rule;
    std::size_t depth;
    bool writes_outside;
  };

  struct Snapshot {
    core::Pointer origin;
    core::JSON::String original;
    core::JSON::String destination;
    core::JSON::String fragment;
    core::Pointer target;
    std::size_t target_offset;
  };

  // Where a rule moved a position, in absolute terms, so that a reference
  // naming anything under it can be followed without asking which rule did it
  std::vector<SchemaTransformRule::Relocation> journal;

  bool asserted{false};

  // A pass that leaves the document as it found it has nothing left to
  // contribute, and running another would schedule the very same work and
  // decline it again. Ending the loop on that rather than on a count is what
  // makes a rule whose condition claims work its transform then declines
  // impossible to spin on
  core::JSON previous{sourcemeta::core::JSON::make_object()};
  bool first_pass{true};

  // One pass per rung, plus a pass for what the ladder leaves behind, is the
  // most a correct set of rules can need. Reaching this means two passes are
  // undoing each other, which the check above cannot see, so it is worth
  // saying out loud where the asserts are compiled in
  constexpr auto MAXIMUM_PASSES{LADDER_DIALECTS.size() + 2};
  std::size_t passes{0};

  while (true) {
    if (schema.is_boolean()) {
      break;
    }

    if (!first_pass && schema == previous) {
      break;
    }

    previous = schema;
    first_pass = false;

    passes += 1;
    assert(passes <= MAXIMUM_PASSES);
    if (passes > MAXIMUM_PASSES) {
      break;
    }

    std::vector<Scheduled> scheduled;
    std::vector<Snapshot> snapshots;

    // Everything the frame has to say is taken down here, while it still
    // describes the document in front of it. A `SchemaFrame::Location` hands
    // out views into the frame and the document, so reading one after an edit
    // is not a stale answer but a dangling one, and the same goes for asking
    // the frame anything else. Past this block nothing consults it
    {
      const core::SchemaFrame frame{
          core::SchemaFrame::Mode::References,
          schema,
          walker,
          resolver,
          default_dialect,
          default_id,
          sourcemeta::core::SchemaFrame::IdentifierMode::Fallback};

      if (assert_convertible && !asserted) {
        assert_convertible_dialects(schema, frame, default_id);
        assert_convertible_metaschema(schema, frame);
        assert_schema_references(frame);
        asserted = true;
      }

      for (const auto &[rule, writes_outside] : rules) {
        rule->begin_pass();
      }

      std::unordered_set<core::Pointer, core::Pointer::Hasher> visited;
      frame.for_each_subschema(
          [&](const core::SchemaFrame::Location &location) -> void {
            auto pointer{core::to_pointer(location.pointer)};
            if (!visited.insert(pointer).second) {
              return;
            }

            const auto &current{core::get(schema, pointer)};
            const auto &vocabularies{frame.vocabularies(location, resolver)};
            Site site{.pointer = std::move(pointer),
                      .dialect = core::JSON::String{location.dialect},
                      .base_dialect = location.base_dialect,
                      .type = location.type,
                      .relative_pointer = location.relative_pointer};

            for (std::size_t index = 0; index < rules.size(); index += 1) {
              const auto &[rule, writes_outside] = rules.at(index);

              // A dialect the ladder does not name has no rules for moving a
              // schema off it, so nothing may rewrite a subschema that is read
              // as one. Leaving this to each rule's own vocabulary gate does
              // not hold: core derives a pre-2019-09 dialect's vocabularies
              // from its base dialect, so an off-ladder resource does carry
              // the rung's vocabulary and does match those gates
              if (!writes_outside && !site.dialect.empty() &&
                  !names_ladder_dialect(site.dialect)) {
                continue;
              }

              // Whatever this rule needs the frame for, it takes down now
              rule->plan(current, schema, vocabularies, frame, location, site,
                         walker, resolver);

              if (rule->condition(current, schema, vocabularies, site, walker,
                                  resolver)) {
                scheduled.push_back({.site = site,
                                     .vocabularies = vocabularies,
                                     .rule = index,
                                     .depth = site.pointer.size(),
                                     .writes_outside = writes_outside});
              }
            }
          });

      frame.for_each_reference(
          [&](const core::SchemaReferenceType, const core::WeakPointer &origin,
              const core::SchemaFrame::Reference &reference) -> void {
            const auto destination{frame.traverse(reference.destination)};
            if (!destination.has_value() || !reference.fragment.has_value() ||
                !reference.fragment.value().starts_with('/')) {
              return;
            }

            const auto &landing{destination.value().get()};

            // A fragment shaped like a pointer is not necessarily one. Draft 4
            // placed no restriction on the fragment an identifier carries, so
            // an anchor may be named something like `/definitions/x`, and
            // framing hands that name the URI the pointer would otherwise have
            // had. Such a reference follows the anchor when it moves rather
            // than keeping the location it looks like it names
            if (landing.type == core::SchemaFrame::LocationType::Anchor) {
              return;
            }

            snapshots.push_back(
                {.origin = core::to_pointer(origin),
                 .original = core::JSON::String{reference.original},
                 .destination = reference.destination,
                 .fragment = core::JSON::String{reference.fragment.value()},
                 .target = core::to_pointer(landing.pointer),
                 .target_offset = landing.relative_pointer});
          });
    }

    if (scheduled.empty()) {
      break;
    }

    // A rule that writes across the document goes first, because what it
    // planned names positions as they are now and any other edit would move
    // them. After that, deepest first, and within one position in the order the
    // rules were registered. Applying what is deepest first is what lets a rule
    // stop asking whether the subschemas below it are still waiting: by the
    // time a position is reached, everything under it has been handled already
    std::ranges::stable_sort(
        scheduled, [](const auto &left, const auto &right) -> bool {
          if (left.writes_outside != right.writes_outside) {
            return left.writes_outside;
          }

          if (left.depth != right.depth) {
            return left.depth > right.depth;
          }

          return left.rule < right.rule;
        });

    journal.clear();
    for (const auto &entry : scheduled) {
      const auto &[rule, writes_outside] = rules.at(entry.rule);

      // An edit applied earlier in this pass may have moved or removed the
      // position a later one was scheduled on
      if (core::try_get(schema, entry.site.pointer) == nullptr) {
        continue;
      }

      auto &current{core::get(schema, entry.site.pointer)};

      // Asked again right before the edit, so that an edit applied earlier in
      // this pass can settle what this one was going to do. The question is
      // answered from what was taken down above, never from the frame
      if (!rule->condition(current, schema, entry.vocabularies, entry.site,
                           walker, resolver)) {
        continue;
      }

      rule->transform(current, entry.site);

      for (const auto &[before, after] : rule->relocations()) {
        journal.emplace_back(entry.site.pointer.concat(before),
                             entry.site.pointer.concat(after));
      }
    }

    repair_references(schema, snapshots, journal, walker, resolver,
                      default_dialect, default_id);
  }
}

#include "rules/definitions_to_defs.h"
#include "rules/dependencies_to_dependent.h"
#include "rules/dialect_override_becomes_dollar_schema.h"
#include "rules/draft_official_dialect_with_https.h"
#include "rules/draft_official_dialect_without_empty_fragment.h"
#include "rules/empty_object_as_true.h"
#include "rules/enum_to_const.h"
#include "rules/modern_official_dialect_with_empty_fragment.h"
#include "rules/modern_official_dialect_with_http.h"
#include "rules/openapi_official_dialect_with_date.h"
#include "rules/openapi_official_dialect_with_empty_fragment.h"
#include "rules/prefix_promoted_2020_12_keywords.h"
#include "rules/prefix_promoted_draft_2019_09_keywords.h"
#include "rules/prefix_promoted_draft_4_keywords.h"
#include "rules/prefix_promoted_draft_6_keywords.h"
#include "rules/prefix_promoted_draft_7_keywords.h"
#include "rules/prefix_promoted_openapi_3_1_keywords.h"
#include "rules/sanitize_draft_4_anchors.h"
#include "rules/shadow_stray_dialect_declaration.h"
#include "rules/upgrade_2019_09_to_2020_12.h"
#include "rules/upgrade_2020_12_to_openapi_3_1.h"
#include "rules/upgrade_draft_3_to_draft_4.h"
#include "rules/upgrade_draft_4_to_draft_6.h"
#include "rules/upgrade_draft_6_to_draft_7.h"
#include "rules/upgrade_draft_7_to_draft_2019_09.h"
#include "rules/upgrade_openapi_3_1_to_openapi_3_2.h"

#undef ONLY_CONTINUE_IF

} // namespace

auto convert(sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const ConvertTarget target, const std::string_view default_dialect,
             const std::string_view default_id) -> void {
  // How a dialect is spelled is settled before any rung is asked about one. A
  // rung rule reads the dialect of the position it is looking at, and a
  // spelling the ladder does not name is one it cannot answer for, so these
  // run to completion first rather than alongside
  std::vector<Rule> spellings;
  spellings.reserve(8);
  spellings.push_back(make_rule<DialectOverrideBecomesDollarSchema>());
  spellings.push_back(make_rule<ShadowStrayDialectDeclaration>());
  spellings.push_back(make_rule<DraftOfficialDialectWithHttps>());
  spellings.push_back(make_rule<DraftOfficialDialectWithoutEmptyFragment>());
  spellings.push_back(make_rule<ModernOfficialDialectWithEmptyFragment>());
  spellings.push_back(make_rule<ModernOfficialDialectWithHttp>());
  spellings.push_back(make_rule<OpenAPIOfficialDialectWithDate>());
  spellings.push_back(make_rule<OpenAPIOfficialDialectWithEmptyFragment>());

  std::vector<Rule> rules;
  rules.reserve(20);
  rules.push_back(make_rule<PrefixPromotedDraft4Keywords>());
  rules.push_back(make_rule<UpgradeDraft3ToDraft4>());

  if (target == ConvertTarget::Draft6 || target == ConvertTarget::Draft7 ||
      target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012 ||
      target == ConvertTarget::OpenAPI31 ||
      target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<PrefixPromotedDraft6Keywords>());
    rules.push_back(make_rule<SanitizeDraft4Anchors>());
    rules.push_back(make_rule<UpgradeDraft4ToDraft6>());
    rules.push_back(make_rule<EmptyObjectAsTrue>());
    rules.push_back(make_rule<EnumToConst>());
  }

  if (target == ConvertTarget::Draft7 || target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012 ||
      target == ConvertTarget::OpenAPI31 ||
      target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<PrefixPromotedDraft7Keywords>());
    rules.push_back(make_rule<UpgradeDraft6ToDraft7>());
  }

  if (target == ConvertTarget::Draft201909 ||
      target == ConvertTarget::Draft202012 ||
      target == ConvertTarget::OpenAPI31 ||
      target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<PrefixPromoted201909Keywords>());
    rules.push_back(make_rule<UpgradeDraft7To201909>());
    rules.push_back(make_rule<DefinitionsToDefs>());
    rules.push_back(make_rule<DependenciesToDependent>());
  }

  if (target == ConvertTarget::Draft202012 ||
      target == ConvertTarget::OpenAPI31 ||
      target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<PrefixPromoted202012Keywords>());
    rules.push_back(make_rule<Upgrade201909To202012>());
  }

  if (target == ConvertTarget::OpenAPI31 ||
      target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<PrefixPromotedOpenAPI31Keywords>());
    rules.push_back(make_rule<Upgrade202012ToOpenAPI31>());
  }

  if (target == ConvertTarget::OpenAPI32) {
    rules.push_back(make_rule<UpgradeOpenAPI31ToOpenAPI32>());
  }

  apply(spellings, schema, walker, resolver, default_dialect, default_id, true);
  apply(rules, schema, walker, resolver, default_dialect, default_id, false);
}

} // namespace sourcemeta::blaze
