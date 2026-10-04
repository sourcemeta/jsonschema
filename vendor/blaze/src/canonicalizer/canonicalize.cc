#include <sourcemeta/blaze/canonicalizer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include "schema_helpers.h"

#include <algorithm>     // std::sort, std::unique, std::ranges::contains,
                         // std::ranges::none_of, std::ranges::sort,
                         // std::ranges::find, std::ranges::find_if
#include <array>         // std::array
#include <bit>           // std::popcount
#include <cassert>       // assert
#include <cmath>         // std::floor, std::ceil, std::isfinite
#include <concepts>      // std::derived_from
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint64_t
#include <functional>    // std::hash, std::ref
#include <limits>        // std::numeric_limits
#include <map>           // std::map
#include <memory>        // std::make_unique, std::unique_ptr
#include <optional>      // std::optional, std::nullopt
#include <set>           // std::set
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <type_traits>   // std::is_same_v, std::true_type
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::to_underlying
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
            const auto outcome{rule->condition(current, schema,
                                               current_vocabularies, *frame,
                                               location, walker, resolver)};

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
              core::set(schema, effective_origin,
                        core::JSON{original.recompose()});
              references_fixed = true;
            }

            const auto new_vocabularies{
                frame->vocabularies(new_location.value().get(), resolver)};

            assert(!rule->condition(current, schema, new_vocabularies, *frame,
                                    new_location.value().get(), walker,
                                    resolver));

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

auto is_draft3(const core::SchemaBaseDialect base_dialect) -> bool {
  return base_dialect == core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3 ||
         base_dialect == core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER;
}

/// Remove every Draft 3 identifier, including anchors and the identifier of
/// the root. Every reference is first resolved while the frame still knows
/// the identifiers, and rewritten into a form that does not depend on any of
/// them: a JSON Pointer into this document when the destination is local, or
/// the absolute URI when it is not. Only then are the identifiers erased, so
/// no reference can lose the base it was resolved against.
///
/// This only applies to documents that are Draft 3 throughout. A document of
/// another dialect may embed a Draft 3 resource and reference it by its
/// identifier from outside, with a reference this pass must not rewrite, so
/// such documents are left untouched
auto eliminate_identifiers(sourcemeta::core::JSON &schema,
                           const sourcemeta::core::SchemaWalker &walker,
                           const sourcemeta::core::SchemaResolver &resolver,
                           const std::string_view default_dialect,
                           const std::string_view default_id) -> void {
  if (!schema.is_object()) {
    return;
  }

  std::vector<std::pair<core::Pointer, core::JSON::String>> reference_changes;
  std::vector<core::Pointer> identifier_owners;

  {
    const core::SchemaFrame frame{core::SchemaFrame::Mode::References,
                                  schema,
                                  walker,
                                  resolver,
                                  default_dialect,
                                  default_id,
                                  core::SchemaFrame::IdentifierMode::Fallback};

    if (frame.any_subschema(
            [](const core::SchemaFrame::Location &location) -> bool {
              return !is_draft3(location.base_dialect);
            })) {
      return;
    }

    frame.for_each_reference(
        [&](const core::SchemaReferenceType, const core::WeakPointer &origin,
            const core::SchemaFrame::Reference &reference) -> void {
          assert(!origin.empty() && origin.back().is_property());
          if (origin.back().to_property() != "$ref") {
            return;
          }

          const auto destination{frame.traverse(reference.destination)};
          reference_changes.emplace_back(
              core::to_pointer(origin),
              destination.has_value()
                  ? core::to_uri(destination->get().pointer).recompose()
                  : reference.destination);
        });

    frame.for_each_subschema(
        [&](const core::SchemaFrame::Location &location) -> void {
          const auto &subschema{core::get(schema, location.pointer)};
          if (subschema.is_object() && subschema.defines("id")) {
            identifier_owners.push_back(core::to_pointer(location.pointer));
          }
        });
  }

  for (const auto &[pointer, value] : reference_changes) {
    core::set(schema, pointer, core::JSON{value});
  }

  // Rewriting a reference cannot turn a subschema into something else
  for (const auto &pointer : identifier_owners) {
    auto &subschema{core::get(schema, pointer)};
    assert(subschema.is_object());
    subschema.erase("id");
  }
}

/// Order two pointers by their tokens, comparing array positions as numbers
/// rather than as text, so that `/type/2` comes before `/type/10`
auto pointer_before(const sourcemeta::core::Pointer &left,
                    const sourcemeta::core::Pointer &right) -> bool {
  for (std::size_t index = 0; index < left.size() && index < right.size();
       index++) {
    const auto &first{left.at(index)};
    const auto &second{right.at(index)};
    if (first.is_property() != second.is_property()) {
      return second.is_property();
    }

    if (first.is_property()) {
      if (first.to_property() != second.to_property()) {
        return first.to_property() < second.to_property();
      }
    } else if (first.to_index() != second.to_index()) {
      return first.to_index() < second.to_index();
    }
  }

  return left.size() < right.size();
}

/// Whether the given reference points at an entry this pass produced. A
/// reference that leaves the document keeps its absolute URI instead
auto entry_number(const sourcemeta::core::JSON &reference)
    -> std::optional<std::size_t> {
  static const std::string_view PREFIX{"#/definitions/"};
  if (!reference.is_string()) {
    return std::nullopt;
  }

  const auto &value{reference.to_string()};
  if (!value.starts_with(PREFIX)) {
    return std::nullopt;
  }

  const auto suffix{value.substr(PREFIX.size())};
  if (suffix.empty() ||
      suffix.find_first_not_of("0123456789") != std::string::npos) {
    return std::nullopt;
  }

  return static_cast<std::size_t>(std::stoull(suffix));
}

/// The reference an entry amounts to, when the entry is nothing but a
/// reference: either on its own, or behind the one-element `extends` that
/// this pass leaves in place of a lifted subschema. Such a subschema is an
/// edge of the graph rather than a node of it
auto edge_reference(const sourcemeta::core::JSON &body)
    -> const sourcemeta::core::JSON * {
  if (!body.is_object() || body.size() != 1) {
    return nullptr;
  }

  if (body.defines("$ref")) {
    return &body.at("$ref");
  }

  if (!body.defines("extends")) {
    return nullptr;
  }

  const auto &branches{body.at("extends")};
  if (!branches.is_array() || branches.size() != 1) {
    return nullptr;
  }

  const auto &branch{branches.at(0)};
  if (branch.is_object() && branch.size() == 1 && branch.defines("$ref")) {
    return &branch.at("$ref");
  }

  return nullptr;
}

/// Whether the given body is an edge rather than a node
auto is_edge(const sourcemeta::core::JSON &body) -> bool {
  return edge_reference(body) != nullptr;
}

/// What a subschema says, with the keywords that carry the graph itself taken
/// out. `definitions` holds other nodes rather than part of this one, and a
/// `required` marker belongs to the parent `properties`
auto without_containers(sourcemeta::core::JSON body) -> sourcemeta::core::JSON {
  if (body.is_object()) {
    body.erase("$schema");
    body.erase("definitions");
    body.erase("required");
  }

  return body;
}

/// Where following a chain of edges ends up: an entry of the graph, or a
/// reference that leaves the document. A chain that only ever reaches other
/// edges reaches neither, and cannot be expressed as a graph at all
struct Destination {
  std::optional<std::size_t> entry;
  std::optional<sourcemeta::core::JSON::String> outside;
};
/// Every place inside a node body where a reference can sit, as a pointer to
/// the `$ref` itself.
///
/// Whether a `$ref` is a reference at all depends on where it sits. The value
/// of an `enum` or a `default`, and of any extension keyword, is instance data
/// rather than a schema, so an object in there that happens to have a `$ref`
/// member says nothing about references and must be left exactly as it is.
/// Looking for `$ref` anywhere in a body would rewrite such a value, or fail
/// on one that names an entry that does not exist.
///
/// Members are read in key order so that the result never depends on the order
/// an object happens to store them in
auto reference_slots(const sourcemeta::core::JSON &body)
    -> std::vector<sourcemeta::core::Pointer> {
  static const std::set<sourcemeta::core::JSON::String> MAPPINGS{
      "properties", "patternProperties", "dependencies"};
  static const std::set<sourcemeta::core::JSON::String> SINGLES{
      "additionalProperties", "additionalItems"};
  // `items` holds either one subschema or a tuple of them, and the three
  // Draft 3 keywords that take branches hold an array once the rules have run,
  // though hyper-schema can leave a lone subschema behind
  static const std::set<sourcemeta::core::JSON::String> BRANCHES{
      "items", "type", "extends", "disallow"};

  std::vector<sourcemeta::core::Pointer> result;
  const auto collect{[&result](const sourcemeta::core::JSON &current,
                               const core::Pointer &position,
                               const auto &self) -> void {
    if (!current.is_object()) {
      return;
    }

    std::vector<sourcemeta::core::JSON::String> keys;
    keys.reserve(current.size());
    for (const auto &member : current.as_object()) {
      keys.push_back(member.first);
    }

    std::ranges::sort(keys);
    for (const auto &key : keys) {
      const auto &value{current.at(key)};
      if (key == "$ref") {
        if (value.is_string()) {
          result.push_back(position.concat(key));
        }
      } else if (MAPPINGS.contains(key) && value.is_object()) {
        std::vector<sourcemeta::core::JSON::String> names;
        names.reserve(value.size());
        for (const auto &member : value.as_object()) {
          names.push_back(member.first);
        }

        std::ranges::sort(names);
        for (const auto &name : names) {
          self(value.at(name), position.concat(key).concat(name), self);
        }
      } else if (SINGLES.contains(key)) {
        self(value, position.concat(key), self);
      } else if (BRANCHES.contains(key)) {
        if (value.is_array()) {
          for (std::size_t index = 0; index < value.size(); index++) {
            self(value.at(index),
                 position.concat(key).concat(
                     static_cast<core::Pointer::Token::Index>(index)),
                 self);
          }
        } else {
          self(value, position.concat(key), self);
        }
      }
    }
  }};

  collect(body, core::Pointer{}, collect);
  return result;
}
/// Rewrite a Draft 3 document into its graph form: every subschema becomes an
/// entry of a single `definitions` at the root, nesting is replaced by
/// references between those entries, and the root becomes a wrapper that
/// references the entry standing for the document.
///
/// Like identifier elimination, this only applies to documents that are Draft 3
/// throughout, and it runs once the rules have finished reshaping the schema
auto lift_subschemas(sourcemeta::core::JSON &schema,
                     const sourcemeta::core::SchemaWalker &walker,
                     const sourcemeta::core::SchemaResolver &resolver,
                     const std::string_view default_dialect,
                     const std::string_view default_id) -> void {
  if (!schema.is_object()) {
    return;
  }

  std::vector<core::Pointer> nodes;
  // Every reference the document declares, before edges are followed through
  std::vector<std::pair<core::Pointer, core::Pointer>> links;
  std::vector<std::pair<core::Pointer, std::size_t>> reference_targets;
  // References whose chain ends outside this document keep their own URI
  std::vector<std::pair<core::Pointer, core::JSON::String>> outside_targets;

  {
    const core::SchemaFrame frame{core::SchemaFrame::Mode::References,
                                  schema,
                                  walker,
                                  resolver,
                                  default_dialect,
                                  default_id,
                                  core::SchemaFrame::IdentifierMode::Fallback};

    // Hyper-schema is left out, unlike identifier elimination above. Its
    // `links` hold link descriptions rather than subschemas, so lifting them
    // into `definitions` and referring to them would say something the
    // document never said. The canonical form this pass produces is the one
    // `canonical-draft3.json` describes, and that is Draft 3 proper
    if (frame.any_subschema(
            [](const core::SchemaFrame::Location &location) -> bool {
              return location.base_dialect !=
                     core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3;
            })) {
      return;
    }

    frame.for_each_subschema(
        [&nodes, &schema](const core::SchemaFrame::Location &location) -> void {
          // The test runs on what the body would be once the container
          // keywords are stripped, so that a root already in graph form is
          // recognised as the wrapper it is
          if (is_edge(
                  without_containers(core::get(schema, location.pointer)))) {
            return;
          }

          auto pointer{core::to_pointer(location.pointer)};
          if (!std::ranges::contains(nodes, pointer)) {
            nodes.push_back(std::move(pointer));
          }
        });

    if (nodes.empty()) {
      return;
    }

    // The frame is free to enumerate subschemas in any order, so fix one here
    // and let nothing downstream depend on which order that was
    std::ranges::sort(nodes, pointer_before);

    frame.for_each_reference(
        [&links, &frame](
            const core::SchemaReferenceType, const core::WeakPointer &origin,
            const core::SchemaFrame::Reference &reference) -> void {
          assert(!origin.empty() && origin.back().is_property());
          if (origin.back().to_property() != "$ref") {
            return;
          }

          const auto destination{frame.traverse(reference.destination)};
          if (destination.has_value()) {
            links.emplace_back(
                core::to_pointer(origin),
                core::to_pointer(destination.value().get().pointer));
          }
        });
  }

  // A reference can target a subschema that is itself an edge, so follow such
  // a chain through to the node it ultimately reaches
  static const core::JSON::String REF_KEYWORD{"$ref"};
  static const core::JSON::String EXTENDS_KEYWORD{"extends"};

  const auto resolve{[&nodes, &links,
                      &schema](const core::Pointer &start) -> Destination {
    auto target{start};
    for (std::size_t hop = 0; hop <= links.size(); hop++) {
      const auto match{std::ranges::find(nodes, target)};
      if (match != nodes.end()) {
        return {.entry = static_cast<std::size_t>(match - nodes.begin()),
                .outside = std::nullopt};
      }

      const auto direct{
          std::ranges::find_if(links, [&target](const auto &entry) -> bool {
            return entry.first == target.concat({REF_KEYWORD});
          })};
      if (direct != links.end()) {
        target = direct->second;
        continue;
      }

      const auto wrapped{
          std::ranges::find_if(links, [&target](const auto &entry) -> bool {
            return entry.first ==
                   target.concat({EXTENDS_KEYWORD,
                                  static_cast<core::Pointer::Token::Index>(0),
                                  REF_KEYWORD});
          })};
      if (wrapped != links.end()) {
        target = wrapped->second;
        continue;
      }

      // Nothing in this document takes the chain any further, so it ends at an
      // edge that points out of it. Whatever referred here has to point there
      // too, since the edge itself is about to go away
      // Held in a named value: the reference points into it, so it has to
      // outlive the check below
      const auto subschema{without_containers(core::get(schema, target))};
      const auto *outside{edge_reference(subschema)};
      // Only a reference that leaves the document can be carried over. One
      // that stays inside it names a subschema this pass is about to take
      // away, and a `#/definitions/...` pointer would then be read back as
      // one of the entries this pass itself named
      if (outside != nullptr && outside->is_string() &&
          !outside->to_string().starts_with('#')) {
        return {.entry = std::nullopt, .outside = outside->to_string()};
      }

      break;
    }

    return {.entry = std::nullopt, .outside = std::nullopt};
  }};

  for (const auto &entry : links) {
    const auto destination{resolve(entry.second)};
    if (destination.entry.has_value()) {
      reference_targets.emplace_back(entry.first, destination.entry.value());
    } else if (destination.outside.has_value()) {
      outside_targets.emplace_back(entry.first, destination.outside.value());
    } else {
      // The chain goes round in circles without ever reaching a subschema that
      // says anything. There is no entry for such a reference to point at, and
      // dropping it would change what the document means, so the graph form
      // does not apply here
      return;
    }
  }

  // The innermost node that contains the given pointer, if any
  const auto enclosing{
      [&nodes](const core::Pointer &pointer) -> std::optional<std::size_t> {
        std::optional<std::size_t> result;
        for (std::size_t index = 0; index < nodes.size(); index++) {
          if (pointer.starts_with(nodes.at(index)) &&
              (!result.has_value() ||
               nodes.at(index).size() > nodes.at(result.value()).size())) {
            result = index;
          }
        }

        return result;
      }};

  const auto link{[](const std::size_t index,
                     const sourcemeta::core::JSON &target,
                     const core::Pointer &position) -> sourcemeta::core::JSON {
    auto reference{sourcemeta::core::JSON::make_object()};
    reference.assign("$ref", sourcemeta::core::JSON{"#/definitions/" +
                                                    std::to_string(index)});

    // `required` is read by the parent `properties`, and Draft 3 ignores
    // the siblings of a `$ref`, so the marker can neither travel into the
    // entry nor sit next to the reference. It stays behind, wrapping it
    const auto *marker{target.is_object() ? target.try_at("required")
                                          : nullptr};
    // Anywhere but a property entry the marker means nothing at all, so
    // a subschema under `items` or `additionalProperties` loses it
    const auto under_properties{
        position.size() >= 2 &&
        position.at(position.size() - 2).is_property() &&
        position.at(position.size() - 2).to_property() == "properties"};
    if (marker == nullptr || !under_properties) {
      return reference;
    }

    auto wrapper{sourcemeta::core::JSON::make_object()};
    wrapper.assign("required", *marker);
    auto branches{sourcemeta::core::JSON::make_array()};
    branches.push_back(std::move(reference));
    wrapper.assign("extends", std::move(branches));
    return wrapper;
  }};

  const auto entry_node{resolve(core::Pointer{}).entry};
  if (!entry_node.has_value()) {
    return;
  }

  const auto root{entry_node.value()};

  // The entries that survive are renamed below, in the order the graph is
  // walked, so the names given here never reach the result. They are simply
  // named by position
  auto definitions{sourcemeta::core::JSON::make_object()};
  for (std::size_t index = 0; index < nodes.size(); index++) {
    const auto &pointer{nodes.at(index)};
    auto body{core::get(schema, pointer)};

    for (std::size_t other = 0; other < nodes.size(); other++) {
      if (other == index || !nodes.at(other).starts_with(pointer) ||
          enclosing(nodes.at(other).initial()) != index) {
        continue;
      }

      const auto position{nodes.at(other).resolve_from(pointer)};
      core::set(body, position,
                link(other, core::get(schema, nodes.at(other)), position));
    }

    for (const auto &entry : reference_targets) {
      if (!entry.first.starts_with(pointer) ||
          enclosing(entry.first.initial()) != index) {
        continue;
      }

      core::set(body, entry.first.resolve_from(pointer),
                sourcemeta::core::JSON{"#/definitions/" +
                                       std::to_string(entry.second)});
    }

    // A reference that ends up outside the document is written out in full,
    // since the edges it used to travel through are not entries of the graph
    for (const auto &entry : outside_targets) {
      if (!entry.first.starts_with(pointer) ||
          enclosing(entry.first.initial()) != index) {
        continue;
      }

      core::set(body, entry.first.resolve_from(pointer),
                sourcemeta::core::JSON{entry.second});
    }

    if (body.is_object()) {
      body.erase("$schema");
      body.erase("required");
      // Every entry of a `definitions` is a node of its own, so the container
      // is not part of any body. Leaving it in would make a later pass lift
      // the same entries again
      body.erase("definitions");
    }

    definitions.assign(std::to_string(index), std::move(body));
  }

  // Collapsing an edge can leave a body that is an edge itself, and so can
  // merging two entries into one, so each step gives the other more to do.
  // Repeat them until neither changes anything, otherwise canonicalising a
  // second time would keep going where the first time stopped
  auto entry_point{root};
  for (std::size_t round = 0; round <= nodes.size(); round++) {
    const auto before{definitions};

    // A body only turns into an edge once its own children have been lifted,
    // so edges are collapsed here rather than when the nodes were chosen:
    // every reference to such an entry is pointed at whatever it referenced,
    // and the entry itself goes away
    std::map<std::size_t, std::size_t> collapsed;
    for (const auto &entry : definitions.as_object()) {
      const auto *reference{edge_reference(entry.second)};
      if (reference == nullptr) {
        continue;
      }

      const auto target{entry_number(*reference)};
      if (target.has_value()) {
        collapsed.emplace(std::stoull(entry.first), target.value());
      }
    }

    const auto destination{[&collapsed](std::size_t index) -> std::size_t {
      for (std::size_t hop = 0; hop <= collapsed.size(); hop++) {
        const auto match{collapsed.find(index)};
        if (match == collapsed.cend()) {
          break;
        }

        index = match->second;
      }

      return index;
    }};

    // What each surviving entry is called once the collapsed ones are gone
    std::map<std::size_t, std::size_t> renamed;
    for (const auto &entry : definitions.as_object()) {
      const auto index{std::stoull(entry.first)};
      if (!collapsed.contains(index)) {
        renamed.emplace(index, renamed.size());
      }
    }

    const auto rewrite{[&destination,
                        &renamed](sourcemeta::core::JSON &body) -> void {
      for (const auto &slot : reference_slots(body)) {
        const auto entry{entry_number(core::get(body, slot))};
        if (!entry.has_value()) {
          continue;
        }

        const auto target{renamed.at(destination(entry.value()))};
        core::set(
            body, slot,
            sourcemeta::core::JSON{"#/definitions/" + std::to_string(target)});
      }
    }};

    auto surviving{sourcemeta::core::JSON::make_object()};
    for (const auto &entry : renamed) {
      auto body{definitions.at(std::to_string(entry.first))};
      rewrite(body);
      surviving.assign(std::to_string(entry.second), std::move(body));
    }

    definitions = std::move(surviving);
    const auto collapsed_entry{renamed.at(destination(entry_point))};

    // Entries that say the same thing become one entry, which is what makes
    // this a graph rather than a tree: every empty schema, for instance, ends
    // up as a single node that everything else references.
    //
    // Two entries say the same thing when their bodies match once references
    // are set aside, and when the entries those references lead to also say the
    // same thing. That is a fixpoint, so start by assuming every body that
    // matches is the same entry, then keep splitting entries whose references
    // disagree until nothing splits any more
    std::vector<sourcemeta::core::JSON> bodies;
    bodies.reserve(definitions.size());
    for (std::size_t index = 0; index < definitions.size(); index++) {
      bodies.push_back(definitions.at(std::to_string(index)));
    }

    // The order the references come out in decides the numbering, so it must
    // not depend on the order an object happens to store its members in.
    // Reading them by sorted key gives the same answer however the object was
    // built
    const auto targets{[](const sourcemeta::core::JSON &body,
                          std::vector<std::size_t> &result) -> void {
      for (const auto &slot : reference_slots(body)) {
        const auto entry{entry_number(core::get(body, slot))};
        if (entry.has_value()) {
          result.push_back(entry.value());
        }
      }
    }};

    // Only the references this pass produced are set aside. A reference that
    // leaves the document names no entry, so it stays part of what the body
    // says: two entries that point at different documents are not the same
    const auto skeleton{
        [](const sourcemeta::core::JSON &body) -> sourcemeta::core::JSON {
          auto result{body};
          for (const auto &slot : reference_slots(result)) {
            if (entry_number(core::get(result, slot)).has_value()) {
              core::set(result, slot, sourcemeta::core::JSON{""});
            }
          }

          return result;
        }};

    std::vector<std::vector<std::size_t>> outgoing{bodies.size()};
    std::vector<std::size_t> classes(bodies.size(), 0);
    std::vector<sourcemeta::core::JSON> shapes;
    for (std::size_t index = 0; index < bodies.size(); index++) {
      targets(bodies.at(index), outgoing.at(index));
      auto shape{skeleton(bodies.at(index))};
      const auto match{std::ranges::find(shapes, shape)};
      if (match == shapes.end()) {
        classes.at(index) = shapes.size();
        shapes.push_back(std::move(shape));
      } else {
        classes.at(index) = static_cast<std::size_t>(match - shapes.begin());
      }
    }

    while (true) {
      std::vector<std::vector<std::size_t>> signatures;
      std::vector<std::size_t> refined(bodies.size(), 0);
      for (std::size_t index = 0; index < bodies.size(); index++) {
        std::vector<std::size_t> signature{classes.at(index)};
        for (const auto target : outgoing.at(index)) {
          signature.push_back(classes.at(target));
        }

        const auto match{std::ranges::find(signatures, signature)};
        if (match == signatures.end()) {
          refined.at(index) = signatures.size();
          signatures.push_back(std::move(signature));
        } else {
          refined.at(index) =
              static_cast<std::size_t>(match - signatures.begin());
        }
      }

      if (refined == classes) {
        break;
      }

      classes = std::move(refined);
    }

    // One entry stands for each group, and the order comes from walking the
    // graph from the entry point, so that it depends on what the nodes say
    // rather than on where they happened to come from
    std::map<std::size_t, std::size_t> representative;
    for (std::size_t index = 0; index < bodies.size(); index++) {
      representative.emplace(classes.at(index), index);
    }

    std::vector<std::size_t> walked;
    const auto walk{[&classes, &outgoing, &representative, &walked](
                        const std::size_t group, const auto &self) -> void {
      if (std::ranges::contains(walked, group)) {
        return;
      }

      walked.push_back(group);
      for (const auto target : outgoing.at(representative.at(group))) {
        self(classes.at(target), self);
      }
    }};

    walk(classes.at(collapsed_entry), walk);
    for (std::size_t index = 0; index < bodies.size(); index++) {
      walk(classes.at(index), walk);
    }

    std::map<std::size_t, std::size_t> position;
    for (std::size_t index = 0; index < walked.size(); index++) {
      position.emplace(walked.at(index), index);
    }

    const auto relabel{[&classes, &position](const sourcemeta::core::JSON &body)
                           -> sourcemeta::core::JSON {
      auto result{body};
      for (const auto &slot : reference_slots(result)) {
        const auto entry{entry_number(core::get(result, slot))};
        if (!entry.has_value()) {
          continue;
        }

        core::set(result, slot,
                  sourcemeta::core::JSON{
                      "#/definitions/" +
                      std::to_string(position.at(classes.at(entry.value())))});
      }

      return result;
    }};

    // Merging entries can leave a disjunction or a conjunction listing the same
    // entry twice, which says nothing new. The rules already drop duplicates in
    // these three keywords, but they run before any of this
    const auto unique{[](sourcemeta::core::JSON &body) -> void {
      if (!body.is_object()) {
        return;
      }

      for (const auto *keyword : {"type", "extends", "disallow"}) {
        auto *entries{body.try_at(keyword)};
        if (entries == nullptr || !entries->is_array()) {
          continue;
        }

        auto result{sourcemeta::core::JSON::make_array()};
        for (const auto &entry : entries->as_array()) {
          bool seen{false};
          for (const auto &kept : result.as_array()) {
            if (kept == entry) {
              seen = true;
              break;
            }
          }

          if (!seen) {
            result.push_back(entry);
          }
        }

        body.assign(keyword, std::move(result));
      }
    }};

    auto merged{sourcemeta::core::JSON::make_object()};
    for (const auto &group : position) {
      auto body{relabel(bodies.at(representative.at(group.first)))};
      unique(body);
      merged.assign(std::to_string(group.second), std::move(body));
    }

    const auto next{position.at(classes.at(collapsed_entry))};
    const auto settled{next == entry_point && merged == before};
    definitions = std::move(merged);
    entry_point = next;
    if (settled) {
      break;
    }
  }

  auto result{sourcemeta::core::JSON::make_object()};
  if (schema.defines("$schema")) {
    result.assign("$schema", schema.at("$schema"));
  }

  auto wrapper{sourcemeta::core::JSON::make_array()};
  auto reference{sourcemeta::core::JSON::make_object()};
  reference.assign("$ref", sourcemeta::core::JSON{"#/definitions/" +
                                                  std::to_string(entry_point)});
  wrapper.push_back(std::move(reference));
  result.assign("extends", std::move(wrapper));
  result.assign("definitions", std::move(definitions));
  schema.into(std::move(result));
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
#include "rules/duplicate_type_entries.h"
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
#include "rules/unknown_type_names.h"
#include "rules/unnecessary_allof_ref_wrapper_draft.h"
#include "rules/unnecessary_extends_ref_wrapper.h"
#include "rules/unsatisfiable_drop_validation.h"
#include "rules/unsatisfiable_empty_enum.h"
#include "rules/unsatisfiable_exclusive_equal_bounds.h"
#include "rules/unsatisfiable_in_place_applicator_type.h"
#include "rules/unsatisfiable_type_and_enum.h"

#undef ONLY_CONTINUE_IF

} // namespace

auto canonicalize(sourcemeta::core::JSON &schema,
                  const sourcemeta::core::SchemaWalker &walker,
                  const sourcemeta::core::SchemaResolver &resolver,
                  const std::string_view default_dialect,
                  const std::string_view default_id) -> void {
  std::vector<Rule> rules;
  rules.reserve(128);
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
  rules.push_back(make_rule<UnknownTypeNames>());
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
  rules.push_back(make_rule<UnsatisfiableEmptyEnum>());
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
  rules.push_back(make_rule<DuplicateTypeEntries>());
  rules.push_back(make_rule<DisallowArrayToExtends>());
  rules.push_back(make_rule<DisallowExtendsToType>());
  rules.push_back(make_rule<DisallowTypeUnionToExtends>());
  rules.push_back(make_rule<DisallowDoubleNegation>());
  rules.push_back(make_rule<RequiredToExtends>());
  rules.push_back(make_rule<SingleBranchAllOf>());
  rules.push_back(make_rule<SingleBranchAnyOf>());
  rules.push_back(make_rule<SingleBranchOneOf>());
  apply(rules, schema, walker, resolver, default_dialect, default_id);

  // Identifiers are removed last, once no rule will reshape the schema again.
  // Removing them first would hand the rules references that are plain JSON
  // Pointers, which some of them do not yet preserve when they move or copy
  // the subschemas those pointers go through
  eliminate_identifiers(schema, walker, resolver, default_dialect, default_id);

  // Lifting comes last: it depends on the identifiers being gone, since a
  // subschema cannot be moved out of a scope that still sets a base URI
  lift_subschemas(schema, walker, resolver, default_dialect, default_id);
}

} // namespace sourcemeta::blaze
