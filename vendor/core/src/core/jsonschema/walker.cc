#include <sourcemeta/core/jsonschema.h>

#include <algorithm> // std::max, std::sort
#include <cassert>   // assert

namespace {
enum class SchemaWalkerType_t : std::uint8_t { Deep, Flat };

auto walk(const std::optional<sourcemeta::core::Pointer> &parent,
          const sourcemeta::core::Pointer &pointer,
          const sourcemeta::core::PointerTemplate &instance_location,
          const sourcemeta::core::PointerTemplate &relative_instance_location,
          std::vector<sourcemeta::core::SchemaIteratorEntry> &subschemas,
          const sourcemeta::core::JSON &subschema,
          const sourcemeta::core::SchemaWalker &walker,
          const sourcemeta::core::SchemaResolver &resolver,
          const std::string &dialect, const SchemaWalkerType_t type,
          const std::size_t level, const bool orphan) -> void {
  if (!is_schema(subschema)) {
    return;
  }

  // Recalculate the dialect and its vocabularies at every step.
  // This is needed for correctly traversing through schemas that
  // contains pointers that use different dialect/vocabularies.
  // This is often the case for bundled schemas.
  const std::optional<std::string> current_dialect{
      sourcemeta::core::dialect(subschema, dialect)};
  assert(current_dialect.has_value());
  const std::string &new_dialect{current_dialect.value()};

  const std::optional<std::string> base_dialect{
      sourcemeta::core::base_dialect(subschema, resolver, new_dialect)};
  assert(base_dialect.has_value());
  const std::map<std::string, bool> vocabularies{sourcemeta::core::vocabularies(
      resolver, base_dialect.value(), new_dialect)};

  if (type == SchemaWalkerType_t::Deep || level > 0) {
    sourcemeta::core::SchemaIteratorEntry entry{
        parent,       pointer,   new_dialect,       vocabularies,
        base_dialect, subschema, instance_location, relative_instance_location,
        orphan};
    subschemas.push_back(std::move(entry));
  }

  // We can't recurse any further
  if (!subschema.is_object() ||
      (type == SchemaWalkerType_t::Flat && level > 0)) {
    return;
  }

  for (auto &pair : subschema.as_object()) {
    switch (walker(pair.first, vocabularies).type) {
      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseSomeProperty: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Condition{});
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Wildcard::Property);
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Condition{},
              sourcemeta::core::PointerTemplate::Wildcard::Property},
             subschemas, pair.second, walker, resolver, new_dialect, type,
             level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseAnyPropertyKey: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Wildcard::Key);
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Wildcard::Key}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseAnyItem: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Wildcard::Item);
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Wildcard::Item}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueTraverseSomeItem: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Condition{});
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Wildcard::Item);
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Condition{},
              sourcemeta::core::PointerTemplate::Wildcard::Item},
             subschemas, pair.second, walker, resolver, new_dialect, type,
             level + 1, orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseParent: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.pop_back();
        walk(pointer, new_pointer, new_instance_location, {}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceOther: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(pointer, new_pointer, instance_location, {}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceNegate: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Negation{});
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Negation{}}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceMaybe: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        auto new_instance_location{instance_location};
        new_instance_location.emplace_back(
            sourcemeta::core::PointerTemplate::Condition{});
        walk(pointer, new_pointer, new_instance_location,
             {sourcemeta::core::PointerTemplate::Condition{}}, subschemas,
             pair.second, walker, resolver, new_dialect, type, level + 1,
             orphan);
      } break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsTraverseItem:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(new_pointer.back());
            walk(pointer, new_pointer, new_instance_location,
                 {new_pointer.back()}, subschemas, pair.second.at(index),
                 walker, resolver, new_dialect, type, level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, instance_location, {}, subschemas,
                 pair.second.at(index), walker, resolver, new_dialect, type,
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
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(
                sourcemeta::core::PointerTemplate::Condition{});
            walk(pointer, new_pointer, new_instance_location,
                 {sourcemeta::core::PointerTemplate::Condition{}}, subschemas,
                 pair.second.at(index), walker, resolver, new_dialect, type,
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
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(
                sourcemeta::core::PointerTemplate::Condition{});
            new_instance_location.emplace_back(
                sourcemeta::core::PointerTemplate::Negation{});
            walk(pointer, new_pointer, new_instance_location,
                 {sourcemeta::core::PointerTemplate::Condition{},
                  sourcemeta::core::PointerTemplate::Negation{}},
                 subschemas, pair.second.at(index), walker, resolver,
                 new_dialect, type, level + 1, orphan);
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
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(new_pointer.back());
            walk(pointer, new_pointer, new_instance_location,
                 {new_pointer.back()}, subschemas, subpair.second, walker,
                 resolver, new_dialect, type, level + 1, orphan);
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
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(subpair.first);
            walk(pointer, new_pointer, new_instance_location, {subpair.first},
                 subschemas, subpair.second, walker, resolver, new_dialect,
                 type, level + 1, orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::ApplicatorMembersInPlaceSome:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(
                sourcemeta::core::PointerTemplate::Condition{});
            walk(pointer, new_pointer, new_instance_location,
                 {sourcemeta::core::PointerTemplate::Condition{}}, subschemas,
                 subpair.second, walker, resolver, new_dialect, type, level + 1,
                 orphan);
          }
        }

        break;

      case sourcemeta::core::SchemaKeywordType::LocationMembers:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(pointer, new_pointer, instance_location, {}, subschemas,
                 subpair.second, walker, resolver, new_dialect, type, level + 1,
                 true);
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
            auto new_instance_location{instance_location};
            new_instance_location.emplace_back(new_pointer.back());
            walk(pointer, new_pointer, new_instance_location,
                 {new_pointer.back()}, subschemas, pair.second.at(index),
                 walker, resolver, new_dialect, type, level + 1, orphan);
          }
        } else {
          sourcemeta::core::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          auto new_instance_location{instance_location};
          new_instance_location.emplace_back(
              sourcemeta::core::PointerTemplate::Wildcard::Item);
          walk(pointer, new_pointer, new_instance_location,
               {sourcemeta::core::PointerTemplate::Wildcard::Item}, subschemas,
               pair.second, walker, resolver, new_dialect, type, level + 1,
               orphan);
        }

        break;

      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueOrElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(pointer, new_pointer, instance_location, {}, subschemas,
                 pair.second.at(index), walker, resolver, new_dialect, type,
                 level + 1, orphan);
          }
        } else {
          sourcemeta::core::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          walk(pointer, new_pointer, instance_location, {}, subschemas,
               pair.second, walker, resolver, new_dialect, type, level + 1,
               orphan);
        }

        break;
      case sourcemeta::core::SchemaKeywordType::Assertion:
        break;
      case sourcemeta::core::SchemaKeywordType::Annotation:
        break;
      case sourcemeta::core::SchemaKeywordType::Reference:
        break;
      case sourcemeta::core::SchemaKeywordType::Other:
        break;
      case sourcemeta::core::SchemaKeywordType::Comment:
        break;
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
  sourcemeta::core::PointerTemplate instance_location;
  // If the given schema declares no dialect and the user didn't
  // not pass a default, then there is nothing we can do. We know
  // the current schema is a subschema, but cannot walk any further.
  if (!dialect.has_value()) {
    sourcemeta::core::SchemaIteratorEntry entry{
        std::nullopt, pointer,           std::nullopt,      {},   std::nullopt,
        schema,       instance_location, instance_location, false};
    this->subschemas.push_back(std::move(entry));
  } else {
    walk(std::nullopt, pointer, instance_location, instance_location,
         this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Deep, 0, false);
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
    sourcemeta::core::PointerTemplate instance_location;
    walk(std::nullopt, pointer, instance_location, instance_location,
         this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Flat, 0, false);
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

  std::map<std::string, bool> vocabularies;
  if (base_dialect.has_value() && dialect.has_value()) {
    vocabularies.merge(sourcemeta::core::vocabularies(
        resolver, base_dialect.value(), dialect.value()));
  }

  for (const auto &entry : schema.as_object()) {
    sourcemeta::core::SchemaIteratorEntry subschema_entry{
        std::nullopt, {entry.first}, dialect, vocabularies,
        base_dialect, entry.second,  {},      {},
        false};
    this->entries.push_back(std::move(subschema_entry));
  }

  // Sort keywords based on priority for correct evaluation
  std::sort(
      this->entries.begin(), this->entries.end(),
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
