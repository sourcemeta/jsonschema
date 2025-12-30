#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::max, std::sort
#include <cassert>   // assert

namespace {
enum class SchemaWalkerType_t : std::uint8_t { Deep, Flat };

auto ref_overrides_adjacent_keywords(const std::string &base_dialect) -> bool {
  // In older drafts, the presence of `$ref` would override any sibling
  // keywords
  // See
  // https://json-schema.org/draft-07/draft-handrews-json-schema-01#rfc.section.8.3
  return base_dialect == "http://json-schema.org/draft-07/schema#" ||
         base_dialect == "http://json-schema.org/draft-07/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-06/schema#" ||
         base_dialect == "http://json-schema.org/draft-06/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-04/schema#" ||
         base_dialect == "http://json-schema.org/draft-04/hyper-schema#" ||
         base_dialect == "http://json-schema.org/draft-03/schema#" ||
         base_dialect == "http://json-schema.org/draft-03/hyper-schema#";
}

auto walk(const std::optional<sourcemeta::core::Pointer> &parent,
          const sourcemeta::core::Pointer &pointer,
          std::vector<sourcemeta::core::SchemaIteratorEntry> &subschemas,
          const sourcemeta::core::JSON &subschema,
          const sourcemeta::core::SchemaWalker &walker,
          const sourcemeta::core::SchemaResolver &resolver,
          const std::string &dialect, const std::string &base_dialect,
          const SchemaWalkerType_t type, const std::size_t level,
          const bool orphan) -> void {
  if (!is_schema(subschema)) {
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
  auto maybe_current_dialect{sourcemeta::core::dialect(subschema, dialect)};
  assert(maybe_current_dialect.has_value());

  // TODO: Note that we determine the identifier here, but the framing does it
  // all over again. Maybe we should be storing this instead?
  auto id{
      sourcemeta::core::identify(subschema, resolver, maybe_current_dialect)};
  const auto different_parent_dialect{maybe_current_dialect.value() != dialect};
  if (!id.has_value() && different_parent_dialect) {
    id = sourcemeta::core::identify(subschema, base_dialect);
    if (id.has_value()) {
      maybe_current_dialect = dialect;
    }
  }

  const auto is_schema_resource{level == 0 || id.has_value()};
  const auto &current_dialect{is_schema_resource ? maybe_current_dialect.value()
                                                 : dialect};
  auto current_base_dialect{
      is_schema_resource && current_dialect != dialect
          ? sourcemeta::core::base_dialect(subschema, resolver, current_dialect)
                .value_or(base_dialect)
          : base_dialect};

  const auto vocabularies{sourcemeta::core::vocabularies(
      resolver, current_base_dialect, current_dialect)};

  if (type == SchemaWalkerType_t::Deep || level > 0) {
    sourcemeta::core::SchemaIteratorEntry entry{.parent = parent,
                                                .pointer = pointer,
                                                .dialect = current_dialect,
                                                .vocabularies = vocabularies,
                                                .base_dialect =
                                                    current_base_dialect,
                                                .subschema = subschema,
                                                .orphan = orphan};
    subschemas.push_back(std::move(entry));
  }

  // We can't recurse any further
  if (!subschema.is_object() ||
      (type == SchemaWalkerType_t::Flat && level > 0)) {
    return;
  }

  const auto has_overriding_ref{
      subschema.defines("$ref") &&
      ref_overrides_adjacent_keywords(current_base_dialect)};
  for (auto &pair : subschema.as_object()) {
    const auto &keyword_info{walker(pair.first, vocabularies)};

    // Ignore the current keyword sibling to `$ref in Draft 7 and older in EVERY
    // case. Note that we purposely DO NOT try to add workarounds for the
    // top-level, `$schema`, or anything else to be purely compliant and avoid
    // lots of gray areas here
    if (has_overriding_ref &&
        keyword_info.type != sourcemeta::core::SchemaKeywordType::Reference) {
      continue;
    }

    switch (keyword_info.type) {
      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseSomeProperty: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseAnyPropertyKey: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseAnyItem: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseSomeItem: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseParent: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceOther: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceNegate: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceMaybe: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
             current_dialect, current_base_dialect, type, level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsTraverseItem:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlaceSome:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorElementsInPlaceSomeNegate:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorMembersTraversePropertyStatic:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorMembersTraversePropertyRegex:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorMembersInPlaceSome:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::LocationMembers:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(pointer, new_pointer, subschemas, subpair.second, walker,
                 resolver, current_dialect, current_base_dialect, type,
                 level + 1, true);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueOrElementsTraverseAnyItemOrItem:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        } else {
          sourcemeta::core::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
               current_dialect, current_base_dialect, type, level + 1, orphan);
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueOrElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, subschemas, pair.second.at(index),
                 walker, resolver, current_dialect, current_base_dialect, type,
                 level + 1, orphan);
          }
        } else {
          sourcemeta::core::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          walk(pointer, new_pointer, subschemas, pair.second, walker, resolver,
               current_dialect, current_base_dialect, type, level + 1, orphan);
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
}
} // namespace

// TODO: These iterators are not very efficient. They traverse once on
// construction and then the client traverses again.

sourcemeta::core::SchemaIterator::SchemaIterator(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  const std::optional<std::string> dialect{
      sourcemeta::core::dialect(schema, default_dialect)};

  sourcemeta::core::Pointer pointer;
  // If the given schema declares no dialect and the user didn't
  // not pass a default, then there is nothing we can do. We know
  // the current schema is a subschema, but cannot walk any further.
  if (!dialect.has_value()) {
    sourcemeta::core::SchemaIteratorEntry entry{.parent = std::nullopt,
                                                .pointer = pointer,
                                                .dialect = std::nullopt,
                                                .vocabularies = {},
                                                .base_dialect = std::nullopt,
                                                .subschema = schema,
                                                .orphan = false};
    this->subschemas.push_back(std::move(entry));
  } else {
    const auto base_dialect{
        sourcemeta::core::base_dialect(schema, resolver, dialect)};
    assert(base_dialect.has_value());
    walk(std::nullopt, pointer, this->subschemas, schema, walker, resolver,
         dialect.value(), base_dialect.value(), SchemaWalkerType_t::Deep, 0,
         false);
  }
}

sourcemeta::core::SchemaIteratorFlat::SchemaIteratorFlat(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  const std::optional<std::string> dialect{
      sourcemeta::core::dialect(schema, default_dialect)};
  if (dialect.has_value()) {
    sourcemeta::core::Pointer pointer;
    const auto base_dialect{
        sourcemeta::core::base_dialect(schema, resolver, dialect)};
    assert(base_dialect.has_value());
    walk(std::nullopt, pointer, this->subschemas, schema, walker, resolver,
         dialect.value(), base_dialect.value(), SchemaWalkerType_t::Flat, 0,
         false);
  }
}

sourcemeta::core::SchemaKeywordIterator::SchemaKeywordIterator(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  assert(is_schema(schema));
  if (schema.is_boolean()) {
    return;
  }

  const std::optional<std::string> dialect{
      sourcemeta::core::dialect(schema, default_dialect)};
  const std::optional<std::string> base_dialect{
      sourcemeta::core::base_dialect(schema, resolver, dialect)};

  Vocabularies vocabularies{
      base_dialect.has_value() && dialect.has_value()
          ? sourcemeta::core::vocabularies(resolver, base_dialect.value(),
                                           dialect.value())
          : Vocabularies{}};

  for (const auto &entry : schema.as_object()) {
    sourcemeta::core::SchemaIteratorEntry subschema_entry{
        .parent = std::nullopt,
        .pointer = {entry.first},
        .dialect = dialect,
        .vocabularies = vocabularies,
        .base_dialect = base_dialect,
        .subschema = entry.second,
        .orphan = false};
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

auto sourcemeta::core::SchemaIterator::begin() const -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::core::SchemaIterator::end() const -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::core::SchemaIterator::cbegin() const -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::core::SchemaIterator::cend() const -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::core::SchemaIteratorFlat::begin() const -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::core::SchemaIteratorFlat::end() const -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::core::SchemaIteratorFlat::cbegin() const -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::core::SchemaIteratorFlat::cend() const -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::core::SchemaKeywordIterator::begin() const -> const_iterator {
  return this->entries.begin();
}
auto sourcemeta::core::SchemaKeywordIterator::end() const -> const_iterator {
  return this->entries.end();
}
auto sourcemeta::core::SchemaKeywordIterator::cbegin() const -> const_iterator {
  return this->entries.cbegin();
}
auto sourcemeta::core::SchemaKeywordIterator::cend() const -> const_iterator {
  return this->entries.cend();
}
