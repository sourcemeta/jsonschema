#include <sourcemeta/blaze/foundation.h>

#include "helpers.h"

#include <algorithm> // std::max, std::sort
#include <cassert>   // assert

namespace {
enum class SchemaWalkerType_t : std::uint8_t { Deep, Flat };

struct DialectInfo {
  std::string_view dialect;
  sourcemeta::blaze::SchemaBaseDialect base_dialect;
  bool override_active;
};

auto resolve_dialect_at(
    const sourcemeta::core::JSON &subschema,
    const std::string_view inherited_dialect,
    const sourcemeta::blaze::SchemaBaseDialect inherited_base,
    const sourcemeta::blaze::SchemaResolver &resolver, const std::size_t level,
    const bool allow_dialect_override) -> DialectInfo {
  auto local{sourcemeta::blaze::dialect(subschema, inherited_dialect,
                                        allow_dialect_override)};
  const auto override_active{
      local != sourcemeta::blaze::dialect(subschema, inherited_dialect, false)};
  auto id{sourcemeta::blaze::identify(subschema, resolver, local, "",
                                      allow_dialect_override)};
  if (id.empty() && local != inherited_dialect && !override_active) {
    id = sourcemeta::blaze::identify(subschema, inherited_base);
    if (!id.empty()) {
      local = inherited_dialect;
    }
  }
  if (!override_active && level > 0 && id.empty()) {
    return {.dialect = inherited_dialect,
            .base_dialect = inherited_base,
            .override_active = false};
  }
  const auto resolved_base{
      local != inherited_dialect
          ? sourcemeta::blaze::base_dialect(subschema, resolver, local,
                                            allow_dialect_override)
                .value_or(inherited_base)
          : inherited_base};
  return {.dialect = local,
          .base_dialect = resolved_base,
          .override_active = override_active};
}

auto walk(const std::optional<sourcemeta::core::WeakPointer> &parent,
          const sourcemeta::core::WeakPointer &pointer,
          std::vector<sourcemeta::blaze::SchemaIteratorEntry> &subschemas,
          const sourcemeta::core::JSON &subschema,
          const sourcemeta::blaze::SchemaWalker &walker,
          const sourcemeta::blaze::SchemaResolver &resolver,
          const std::string_view dialect,
          const sourcemeta::blaze::SchemaBaseDialect base_dialect,
          const SchemaWalkerType_t type, const std::size_t level,
          const bool orphan, const bool property_name) -> void {
  if (!sourcemeta::blaze::is_schema(subschema)) {
    return;
  }

  // Recalculate the dialect and its vocabularies at every step.
  // This is needed for correctly traversing through schemas that
  // contains pointers that use different dialect/vocabularies.
  // This is often the case for bundled schemas.

  // However, we need to be careful with not considering `$schema` on subschemas
  // that do not represent schema resources, as this is not allowed in JSON
  // Schema. See
  // https://json-schema.org/draft/2019-09/draft-handrews-json-schema-02#rfc.section.8.1.1
  // To play it safe, in those cases, we will continue with the current dialect
  // / base dialect and ignore the invalid standalone `$schema`. The caller has
  // enough information to detect those cases and throw an error if they desire
  // to be more strict.

  const auto enclosing_ref_overrides{
      subschema.is_object() && subschema.defines("$ref") &&
      sourcemeta::blaze::ref_overrides_adjacent_keywords(base_dialect)};

  const auto entry{resolve_dialect_at(subschema, dialect, base_dialect,
                                      resolver, level,
                                      !enclosing_ref_overrides)};
  const auto current_dialect{entry.dialect};
  const auto current_base_dialect{entry.base_dialect};

  // A subschema may be an embedded resource that pins its own custom
  // meta-schema inside its own `$defs`/`definitions`. Probe for it here, the
  // same way we do at the document root, so that nested self-contained
  // meta-schemas resolve to their embedded definition before the resolver
  const auto vocabularies{sourcemeta::blaze::vocabularies(
      [&subschema, &resolver](const std::string_view identifier)
          -> std::optional<sourcemeta::core::JSON> {
        const auto *embedded{sourcemeta::blaze::metaschema_try_embedded(
            subschema, identifier, resolver)};
        if (embedded) {
          return *embedded;
        }

        return resolver(identifier);
      },
      current_base_dialect, current_dialect)};

  if (type == SchemaWalkerType_t::Deep || level > 0) {
    sourcemeta::blaze::SchemaIteratorEntry iterator_entry{
        .parent = parent,
        .pointer = pointer,
        .dialect = current_dialect,
        .vocabularies = vocabularies,
        .base_dialect = current_base_dialect,
        .subschema = subschema,
        .orphan = orphan,
        .property_name = property_name};
    subschemas.push_back(std::move(iterator_entry));
  }

  // We can't recurse any further
  if (!subschema.is_object() ||
      (type == SchemaWalkerType_t::Flat && level > 0)) {
    return;
  }

  const auto child{entry.override_active
                       ? resolve_dialect_at(subschema, dialect, base_dialect,
                                            resolver, level, false)
                       : entry};
  const auto child_dialect{child.dialect};
  const auto child_base_dialect{child.base_dialect};

  const auto has_overriding_ref{
      subschema.defines("$ref") &&
      sourcemeta::blaze::ref_overrides_adjacent_keywords(current_base_dialect)};
  for (auto &pair : subschema.as_object()) {
    const auto &keyword_info{walker(pair.first, vocabularies)};

    // Ignore the current keyword sibling to `$ref in Draft 7 and older in EVERY
    // case. Note that we purposely DO NOT try to add workarounds for the
    // top-level, `$schema`, or anything else to be purely compliant and avoid
    // lots of gray areas here
    if (has_overriding_ref &&
        keyword_info.type != sourcemeta::blaze::SchemaKeywordType::Reference) {
      continue;
    }

    switch (keyword_info.type) {
      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueTraverseSomeProperty: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan, false);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueTraverseAnyPropertyKey: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan, true);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueTraverseAnyItem: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan, false);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueTraverseSomeItem: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan, false);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueTraverseParent: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan, false);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorValueInPlaceOther: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan,
             property_name);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorValueInPlaceNegate: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan,
             property_name);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorValueInPlaceMaybe: {
        sourcemeta::core::WeakPointer new_pointer{pointer};
        new_pointer.push_back(std::cref(pair.first));
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             child_dialect, child_base_dialect, type, level + 1, orphan,
             property_name);
      } break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorElementsTraverseItem:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, false);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, property_name);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorElementsInPlaceSome:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, property_name);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorElementsInPlaceSomeNegate:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, property_name);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorMembersTraversePropertyStatic:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.push_back(std::cref(subpair.first));
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, child_dialect, child_base_dialect, type, level + 1,
                 orphan, false);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorMembersTraversePropertyRegex:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.push_back(std::cref(subpair.first));
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, child_dialect, child_base_dialect, type, level + 1,
                 orphan, false);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::ApplicatorMembersInPlaceSome:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.push_back(std::cref(subpair.first));
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, child_dialect, child_base_dialect, type, level + 1,
                 orphan, property_name);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::LocationMembers:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.push_back(std::cref(subpair.first));
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, child_dialect, child_base_dialect, type, level + 1,
                 true, false);
          }
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueOrElementsTraverseAnyItemOrItem:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, false);
          }
        } else {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
               child_dialect, child_base_dialect, type, level + 1, orphan,
               false);
        }

        break;

      case sourcemeta::blaze::SchemaKeywordType::
          ApplicatorValueOrElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::WeakPointer new_pointer{pointer};
            new_pointer.push_back(std::cref(pair.first));
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, child_dialect, child_base_dialect, type,
                 level + 1, orphan, property_name);
          }
        } else {
          sourcemeta::core::WeakPointer new_pointer{pointer};
          new_pointer.push_back(std::cref(pair.first));
          walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
               child_dialect, child_base_dialect, type, level + 1, orphan,
               property_name);
        }

        break;
      case sourcemeta::blaze::SchemaKeywordType::Assertion:
      case sourcemeta::blaze::SchemaKeywordType::Annotation:
      case sourcemeta::blaze::SchemaKeywordType::Reference:
      case sourcemeta::blaze::SchemaKeywordType::Other:
      case sourcemeta::blaze::SchemaKeywordType::Comment:
      case sourcemeta::blaze::SchemaKeywordType::Unknown:
        break;
    }
  }
}
} // namespace

// TODO: These iterators are not very efficient. They traverse once on
// construction and then the client traverses again.

sourcemeta::blaze::SchemaIterator::SchemaIterator(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    std::string_view default_dialect) {
  const std::string_view resolved_dialect{
      sourcemeta::blaze::dialect(schema, default_dialect)};

  sourcemeta::core::WeakPointer pointer;
  // If the given schema declares no dialect and the user didn't
  // not pass a default, then there is nothing we can do. We know
  // the current schema is a subschema, but cannot walk any further.
  if (resolved_dialect.empty()) {
    sourcemeta::blaze::SchemaIteratorEntry entry{.parent = std::nullopt,
                                                 .pointer = pointer,
                                                 .dialect = "",
                                                 .vocabularies = {},
                                                 .base_dialect = std::nullopt,
                                                 .subschema = schema,
                                                 .orphan = false,
                                                 .property_name = false};
    this->subschemas.push_back(std::move(entry));
  } else {
    const auto resolved_base_dialect{
        sourcemeta::blaze::base_dialect(schema, resolver, resolved_dialect)};
    assert(resolved_base_dialect.has_value());
    walk(std::nullopt, pointer, this->subschemas, schema, walker, resolver,
         resolved_dialect, resolved_base_dialect.value(),
         SchemaWalkerType_t::Deep, 0, false, false);
  }
}

sourcemeta::blaze::SchemaIteratorFlat::SchemaIteratorFlat(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::SchemaResolver &resolver,
    const std::string_view default_dialect) {
  const std::string_view resolved_dialect{
      sourcemeta::blaze::dialect(schema, default_dialect)};
  if (!resolved_dialect.empty()) {
    sourcemeta::core::WeakPointer pointer;
    const auto resolved_base_dialect{
        sourcemeta::blaze::base_dialect(schema, resolver, resolved_dialect)};
    assert(resolved_base_dialect.has_value());
    walk(std::nullopt, pointer, this->subschemas, schema, walker, resolver,
         resolved_dialect, resolved_base_dialect.value(),
         SchemaWalkerType_t::Flat, 0, false, false);
  }
}

sourcemeta::blaze::SchemaKeywordIterator::SchemaKeywordIterator(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::blaze::SchemaWalker &walker,
    const sourcemeta::blaze::Vocabularies &vocabularies) {
  assert(is_schema(schema));
  if (schema.is_boolean()) {
    return;
  }

  // TODO: Use std::ranges::to<std::vector>() once libc++ supports it
  // (__cpp_lib_ranges_to_container)
  for (const auto &entry : schema.as_object()) {
    sourcemeta::core::WeakPointer entry_pointer;
    entry_pointer.push_back(std::cref(entry.first));
    sourcemeta::blaze::SchemaIteratorEntry subschema_entry{
        .parent = std::nullopt,
        .pointer = std::move(entry_pointer),
        .dialect = "",
        .vocabularies = vocabularies,
        .base_dialect = std::nullopt,
        .subschema = entry.second,
        .orphan = false,
        .property_name = false};
    this->entries.push_back(std::move(subschema_entry));
  }

  // Sort keywords based on priority for correct evaluation
  std::ranges::sort(
      this->entries,
      [&vocabularies, &walker](const auto &left, const auto &right) -> bool {
        // These cannot be empty or indexes, as we created
        // the entries array from a JSON object
        assert(!left.pointer.empty() && left.pointer.back().is_property());
        assert(!right.pointer.empty() && right.pointer.back().is_property());

        const auto left_priority = schema_keyword_priority(
            left.pointer.back().to_property(), vocabularies, walker);
        const auto right_priority = schema_keyword_priority(
            right.pointer.back().to_property(), vocabularies, walker);

        // Sort first on priority, second on actual keywords. The latter is to
        // make sure different compilers with different STL implementations end
        // up at the exact same result. Not really mandatory, but useful for
        // writing tests on the iterator output.
        if (left_priority != right_priority) {
          return left_priority < right_priority;
        } else {
          return left.pointer < right.pointer;
        }
      });
}

auto sourcemeta::blaze::SchemaIterator::begin() const -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::blaze::SchemaIterator::end() const -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::blaze::SchemaIterator::cbegin() const -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::blaze::SchemaIterator::cend() const -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::blaze::SchemaIteratorFlat::begin() const -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::blaze::SchemaIteratorFlat::end() const -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::blaze::SchemaIteratorFlat::cbegin() const -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::blaze::SchemaIteratorFlat::cend() const -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::blaze::SchemaKeywordIterator::begin() const -> const_iterator {
  return this->entries.begin();
}
auto sourcemeta::blaze::SchemaKeywordIterator::end() const -> const_iterator {
  return this->entries.end();
}
auto sourcemeta::blaze::SchemaKeywordIterator::cbegin() const
    -> const_iterator {
  return this->entries.cbegin();
}
auto sourcemeta::blaze::SchemaKeywordIterator::cend() const -> const_iterator {
  return this->entries.cend();
}
