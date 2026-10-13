#ifndef SOURCEMETA_CORE_JSONSCHEMA_ITERATOR_H_
#define SOURCEMETA_CORE_JSONSCHEMA_ITERATOR_H_

#include <sourcemeta/core/json.h>

#include <sourcemeta/core/jsonschema.h>

#include "helpers.h"

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <functional>  // std::cref, std::reference_wrapper
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace sourcemeta::core {

/// A subschema as reported by SchemaIterator
struct SubschemaEntry {
  std::optional<sourcemeta::core::WeakPointer> parent;
  sourcemeta::core::WeakPointer pointer;
  std::string_view dialect;
  SchemaVocabularies vocabularies;
  std::optional<SchemaBaseDialect> base_dialect;
  std::reference_wrapper<const sourcemeta::core::JSON> subschema;
  bool orphan;
  bool property_name;
};

struct DialectInfo {
  std::string_view dialect;
  sourcemeta::core::SchemaBaseDialect base_dialect;
  bool override_active;
};

// An identifier only counts where the dialect in force gives the keyword that
// carries it a meaning. Telling costs a vocabulary lookup, so only a schema
// that wrote one ever pays for it
inline auto
identifier_counts(const sourcemeta::core::JSON &subschema,
                  const std::string_view identifier,
                  const std::string_view dialect,
                  const sourcemeta::core::SchemaBaseDialect base_dialect,
                  const sourcemeta::core::SchemaResolver &resolver,
                  const sourcemeta::core::SchemaWalker &walker) -> bool {
  if (identifier.empty()) {
    return true;
  }

  const auto vocabularies{sourcemeta::core::vocabularies_with_embedded(
      subschema, resolver, base_dialect, dialect)};
  return sourcemeta::core::dialect_defines_identifier(walker, vocabularies,
                                                      base_dialect);
}

// A schema names the dialect it is written against through a keyword, which
// has to be readable before the dialect is known, since the dialect is what
// says which keywords mean anything. That bootstrap is only sound while the
// dialect being named is one that goes on to define the keyword that named it.
// A dialect that does not leaves a declaration asserting the conditions under
// which it could not have been written, and no reading of it holds up:
//
// Take the declaration at its word and the keyword carrying it means nothing,
// so the schema never said which dialect it was. Ignore it and the dialect is
// whatever was inherited or supplied from outside, which silently reads the
// schema under rules its author rejected. Where the same subschema also names
// an identifier, ignoring goes further still and keeps a resource boundary
// that the named dialect has no keyword to express.
//
// So there is nothing to choose between, and such a declaration is refused.
// Naming the dialect from outside, which no keyword of the schema has to
// support, remains open to the caller.
//
// What makes this general rather than a rule about one dialect is that the
// question is put to the declaration itself: resolve what the schema named,
// work out the vocabularies that accepting it would bring into force, and ask
// those whether the naming keyword means anything. Asking about the dialect
// named rather than the one that ended up in force also matters, because a
// later fallback can discard the declaration, and a declaration that was
// discarded is exactly the silent case worth refusing
inline auto declaration_counts(const sourcemeta::core::JSON &subschema,
                               const std::string_view declared,
                               const sourcemeta::core::SchemaResolver &resolver,
                               const sourcemeta::core::SchemaWalker &walker)
    -> bool {
  const auto resolve{
      sourcemeta::core::resolver_with_embedded(subschema, resolver)};

  const auto base{
      sourcemeta::core::base_dialect(subschema, resolve, declared, false)};
  if (!base.has_value()) {
    return true;
  }

  return sourcemeta::core::dialect_defines(
      walker, sourcemeta::core::vocabularies(resolve, base.value(), declared),
      "$schema"sv);
}

inline auto
resolve_dialect_at(const sourcemeta::core::JSON &subschema,
                   const std::string_view inherited_dialect,
                   const sourcemeta::core::SchemaBaseDialect inherited_base,
                   const sourcemeta::core::SchemaResolver &resolver,
                   const sourcemeta::core::SchemaWalker &walker,
                   const std::size_t level, const bool allow_dialect_override)
    -> DialectInfo {
  auto local{sourcemeta::core::dialect(subschema, inherited_dialect,
                                       allow_dialect_override)};
  const auto override_active{
      local != sourcemeta::core::dialect(subschema, inherited_dialect, false)};
  auto identifier{sourcemeta::core::identify(subschema, resolver, local,
                                             allow_dialect_override)};
  // Telling whether an identifier counts takes a base dialect, which costs a
  // meta-schema to resolve, so a schema that declares none never asks. Note
  // this is not the base dialect the function goes on to report, which is
  // settled only once the fallback below has had its say
  if (!identifier.empty()) {
    const auto local_base{
        local != inherited_dialect
            ? sourcemeta::core::base_dialect(subschema, resolver, local,
                                             allow_dialect_override)
                  .value_or(inherited_base)
            : inherited_base};
    if (!identifier_counts(subschema, identifier, local, local_base, resolver,
                           walker)) {
      identifier = {};
    }
  }
  if (identifier.empty() && local != inherited_dialect && !override_active) {
    identifier = sourcemeta::core::identify(subschema, inherited_base);
    if (!identifier_counts(subschema, identifier, inherited_dialect,
                           inherited_base, resolver, walker)) {
      identifier = {};
    }
    if (!identifier.empty()) {
      local = inherited_dialect;
    }
  }
  if (!override_active && level > 0 && identifier.empty()) {
    return {.dialect = inherited_dialect,
            .base_dialect = inherited_base,
            .override_active = false};
  }
  const auto resolved_base{
      local != inherited_dialect
          ? sourcemeta::core::base_dialect(subschema, resolver, local,
                                           allow_dialect_override)
                .value_or(inherited_base)
          : inherited_base};
  return {.dialect = local,
          .base_dialect = resolved_base,
          .override_active = override_active};
}

// A subschema that the traversal has discovered but not visited yet
struct PendingSubschema {
  std::optional<sourcemeta::core::WeakPointer> parent;
  sourcemeta::core::WeakPointer pointer;
  std::reference_wrapper<const sourcemeta::core::JSON> subschema;
  std::string_view dialect;
  sourcemeta::core::SchemaBaseDialect base_dialect;
  std::size_t level;
  bool orphan;
  bool property_name;
};

inline auto enqueue(std::vector<PendingSubschema> &children,
                    const sourcemeta::core::WeakPointer &parent,
                    sourcemeta::core::WeakPointer pointer,
                    const sourcemeta::core::JSON &subschema,
                    const std::string_view dialect,
                    const sourcemeta::core::SchemaBaseDialect base_dialect,
                    const std::size_t level, const bool orphan,
                    const bool property_name) -> void {
  children.push_back({.parent = parent,
                      .pointer = std::move(pointer),
                      .subschema = subschema,
                      .dialect = dialect,
                      .base_dialect = base_dialect,
                      .level = level,
                      .orphan = orphan,
                      .property_name = property_name});
}

// Traversal is driven by an explicit work stack rather than recursion, so that
// framing a deeply nested schema is bounded by the heap instead of the call
// stack
inline auto
walk(const std::optional<sourcemeta::core::WeakPointer> &root_parent,
     const sourcemeta::core::WeakPointer &root_pointer,
     std::vector<SubschemaEntry> &subschemas,
     const sourcemeta::core::JSON &root_subschema,
     const sourcemeta::core::SchemaWalker &walker,
     const sourcemeta::core::SchemaResolver &resolver,
     const std::string_view root_dialect,
     const sourcemeta::core::SchemaBaseDialect root_base_dialect,
     const std::size_t root_level, const bool root_orphan,
     const bool root_property_name) -> void {
  std::vector<PendingSubschema> pending;
  pending.push_back({.parent = root_parent,
                     .pointer = root_pointer,
                     .subschema = root_subschema,
                     .dialect = root_dialect,
                     .base_dialect = root_base_dialect,
                     .level = root_level,
                     .orphan = root_orphan,
                     .property_name = root_property_name});

  std::vector<PendingSubschema> children;
  while (!pending.empty()) {
    const auto current{std::move(pending.back())};
    pending.pop_back();

    const auto &parent{current.parent};
    const auto &pointer{current.pointer};
    const auto &subschema{current.subschema.get()};
    const auto dialect{current.dialect};
    const auto base_dialect{current.base_dialect};
    const auto level{current.level};
    const auto orphan{current.orphan};
    const auto property_name{current.property_name};

    if (!subschema.is_object() && !subschema.is_boolean()) {
      continue;
    }

    // Recalculate the dialect and its vocabularies at every step.
    // This is needed for correctly traversing through schemas that
    // contains pointers that use different dialect/vocabularies.
    // This is often the case for bundled schemas.

    // However, we need to be careful with not considering `$schema` on
    // subschemas that do not represent schema resources, as this is not allowed
    // in JSON Schema. See
    // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.1.1
    // To play it safe, in those cases, we will continue with the current
    // dialect / base dialect and ignore the invalid standalone `$schema`. The
    // caller has enough information to detect those cases and throw an error if
    // they desire to be more strict.

    const auto enclosing_ref_overrides{
        subschema.is_object() &&
        subschema.defines("$ref"sv, JSONSCHEMA_HASH_REF) &&
        sourcemeta::core::ref_overrides_adjacent_keywords(base_dialect)};

    const auto entry{resolve_dialect_at(subschema, dialect, base_dialect,
                                        resolver, walker, level,
                                        !enclosing_ref_overrides)};
    const auto current_dialect{entry.dialect};
    const auto current_base_dialect{entry.base_dialect};

    // A subschema may be an embedded resource that pins its own custom
    // meta-schema inside its own `$defs`/`definitions`. Probe for it here, the
    // same way we do at the document root, so that nested self-contained
    // meta-schemas resolve to their embedded definition before the resolver
    const auto vocabularies{sourcemeta::core::vocabularies_with_embedded(
        subschema, resolver, current_base_dialect, current_dialect)};

    // A schema that names its dialect through a keyword that very dialect
    // leaves undefined cannot be taken at its word, because honouring it would
    // make the keyword mean something the dialect says it does not. The caller
    // has to name such a dialect from the outside instead
    if (subschema.is_object()) {
      const auto *declared{
          subschema.try_at("$schema"sv, JSONSCHEMA_HASH_SCHEMA)};
      if (declared != nullptr && declared->is_string() &&
          !declaration_counts(subschema, declared->to_string(), resolver,
                              walker)) {
        throw sourcemeta::core::SchemaDialectImpossibleError(
            "$schema", declared->to_string(),
            sourcemeta::core::to_pointer(pointer));
      }
    }

    SubschemaEntry iterator_entry{.parent = parent,
                                  .pointer = pointer,
                                  .dialect = current_dialect,
                                  .vocabularies = vocabularies,
                                  .base_dialect = current_base_dialect,
                                  .subschema = subschema,
                                  .orphan = orphan,
                                  .property_name = property_name};
    subschemas.push_back(std::move(iterator_entry));

    // We can't recurse any further
    if (!subschema.is_object()) {
      continue;
    }

    const auto child{entry.override_active
                         ? resolve_dialect_at(subschema, dialect, base_dialect,
                                              resolver, walker, level, false)
                         : entry};
    const auto child_dialect{child.dialect};
    const auto child_base_dialect{child.base_dialect};

    const auto has_overriding_ref{
        subschema.defines("$ref"sv, JSONSCHEMA_HASH_REF) &&
        sourcemeta::core::ref_overrides_adjacent_keywords(
            current_base_dialect)};
    for (const auto &pair : subschema.as_object()) {
      const auto &keyword_info{walker(pair.first, vocabularies)};

      // Ignore the current keyword sibling to `$ref in Draft 7 and older in
      // EVERY case. Note that we purposely DO NOT try to add workarounds for
      // the top-level, `$schema`, or anything else to be purely compliant and
      // avoid lots of gray areas here
      if (has_overriding_ref &&
          keyword_info.type != sourcemeta::core::SchemaKeywordType::Reference) {
        continue;
      }

      switch (keyword_info.type) {
        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueTraverseSomeProperty: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan, false);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueTraverseAnyPropertyKey: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan, true);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueTraverseAnyItem: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan, false);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueTraverseSomeItem: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan, false);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueTraverseParent: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan, false);
        } break;

        case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceOther: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan,
                  property_name);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueInPlaceNegate: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan,
                  property_name);
        } break;

        case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceMaybe: {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          enqueue(children, pointer, std::move(new_pointer), pair.second,
                  child_dialect, child_base_dialect, level + 1, orphan,
                  property_name);
        } break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorElementsTraverseItem:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, false);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, property_name);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlaceSome:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, property_name);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorElementsInPlaceSomeNegate:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, property_name);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorMembersTraversePropertyStatic:
          if (pair.second.is_object()) {
            for (const auto &subpair : pair.second.as_object()) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.push_back(std::cref(subpair.first));
              enqueue(children, pointer, std::move(new_pointer), subpair.second,
                      child_dialect, child_base_dialect, level + 1, orphan,
                      false);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorMembersTraversePropertyRegex:
          if (pair.second.is_object()) {
            for (const auto &subpair : pair.second.as_object()) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.push_back(std::cref(subpair.first));
              enqueue(children, pointer, std::move(new_pointer), subpair.second,
                      child_dialect, child_base_dialect, level + 1, orphan,
                      false);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::ApplicatorMembersInPlaceSome:
          if (pair.second.is_object()) {
            for (const auto &subpair : pair.second.as_object()) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.push_back(std::cref(subpair.first));
              enqueue(children, pointer, std::move(new_pointer), subpair.second,
                      child_dialect, child_base_dialect, level + 1, orphan,
                      property_name);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::LocationMembers:
          if (pair.second.is_object()) {
            for (const auto &subpair : pair.second.as_object()) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.push_back(std::cref(subpair.first));
              enqueue(children, pointer, std::move(new_pointer), subpair.second,
                      child_dialect, child_base_dialect, level + 1, true,
                      false);
            }
          }

          break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueOrElementsTraverseAnyItemOrItem:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, false);
            }
          } else {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            enqueue(children, pointer, std::move(new_pointer), pair.second,
                    child_dialect, child_base_dialect, level + 1, orphan,
                    false);
          }

          break;

        case sourcemeta::core::SchemaKeywordType::
            ApplicatorValueOrElementsInPlace:
          if (pair.second.is_array()) {
            for (std::size_t index = 0; index < pair.second.size(); index++) {
              sourcemeta::core::WeakPointer new_pointer{pointer};
              new_pointer.push_back(std::cref(pair.first));
              new_pointer.emplace_back(index);
              enqueue(children, pointer, std::move(new_pointer),
                      pair.second.at(index), child_dialect, child_base_dialect,
                      level + 1, orphan, property_name);
            }
          } else {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            enqueue(children, pointer, std::move(new_pointer), pair.second,
                    child_dialect, child_base_dialect, level + 1, orphan,
                    property_name);
          }

          break;
        case sourcemeta::core::SchemaKeywordType::Assertion:
        case sourcemeta::core::SchemaKeywordType::Annotation:
        case sourcemeta::core::SchemaKeywordType::Reference:
        case sourcemeta::core::SchemaKeywordType::Other:
        case sourcemeta::core::SchemaKeywordType::Comment:
        case sourcemeta::core::SchemaKeywordType::Unknown:
          break;
      }
    }

    // Push in reverse so that the first child is popped first, which keeps the
    // resulting order identical to a depth-first recursive traversal
    while (!children.empty()) {
      pending.push_back(std::move(children.back()));
      children.pop_back();
    }
  }
}

/// Iterate over every subschema of a schema, including the schema itself
class SchemaIterator {
private:
  using internal = std::vector<SubschemaEntry>;

public:
  using const_iterator = internal::const_iterator;
  SchemaIterator(const sourcemeta::core::JSON &schema,
                 const SchemaWalker &walker, const SchemaResolver &resolver,
                 std::string_view default_dialect = "");
  [[nodiscard]] auto begin() const -> const_iterator;
  [[nodiscard]] auto end() const -> const_iterator;

private:
  internal subschemas_{};
};

// TODO: This iterator is not very efficient. It traverses once on
// construction and then the client traverses again.

inline SchemaIterator::SchemaIterator(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    std::string_view default_dialect) {
  const std::string_view resolved_dialect{
      sourcemeta::core::dialect(schema, default_dialect)};

  // Whoever walks a schema has already settled what it is written against,
  // either from the schema itself or from the default they handed over, as
  // nothing of the walk can be read without it
  assert(!resolved_dialect.empty());
  const auto resolved_base_dialect{
      sourcemeta::core::base_dialect(schema, resolver, resolved_dialect)};
  assert(resolved_base_dialect.has_value());
  sourcemeta::core::WeakPointer pointer;
  walk(std::nullopt, pointer, this->subschemas_, schema, walker, resolver,
       resolved_dialect, resolved_base_dialect.value(), 0, false, false);
}

inline auto SchemaIterator::begin() const -> const_iterator {
  return this->subschemas_.begin();
}
inline auto SchemaIterator::end() const -> const_iterator {
  return this->subschemas_.end();
}

} // namespace sourcemeta::core

#endif
