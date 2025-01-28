#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/jsonschema_walker.h>

#include <algorithm> // std::max, std::sort
#include <cassert>   // assert
#include <numeric>   // std::accumulate

auto sourcemeta::core::keyword_priority(
    std::string_view keyword, const std::map<std::string, bool> &vocabularies,
    const sourcemeta::core::SchemaWalker &walker) -> std::uint64_t {
  const auto result{walker(keyword, vocabularies)};
  return std::accumulate(
      result.dependencies.cbegin(), result.dependencies.cend(),
      static_cast<std::uint64_t>(0),
      [&vocabularies, &walker](const auto accumulator, const auto &dependency) {
        return std::max(accumulator,
                        keyword_priority(dependency, vocabularies, walker) + 1);
      });
}

namespace {
enum class SchemaWalkerType_t : std::uint8_t { Deep, Flat };

auto walk(sourcemeta::core::Pointer &pointer,
          std::vector<sourcemeta::core::SchemaIteratorEntry> &subschemas,
          const sourcemeta::core::JSON &subschema,
          const sourcemeta::core::SchemaWalker &walker,
          const sourcemeta::core::SchemaResolver &resolver,
          const std::string &dialect, const SchemaWalkerType_t type,
          const std::size_t level) -> void {
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
    subschemas.push_back(
        {pointer, new_dialect, vocabularies, base_dialect, subschema});
  }

  // We can't recurse any further
  if (!subschema.is_object() ||
      (type == SchemaWalkerType_t::Flat && level > 0)) {
    return;
  }

  for (auto &pair : subschema.as_object()) {
    switch (walker(pair.first, vocabularies).type) {
      case sourcemeta::core::KeywordType::ApplicatorValue:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorValueOther:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorValueInPlace: {
        sourcemeta::core::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(new_pointer, subschemas, pair.second, walker, resolver,
             new_dialect, type, level + 1);
      } break;
      case sourcemeta::core::KeywordType::ApplicatorElements:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorElementsInline:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::core::KeywordType::ApplicatorMembers:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorMembersInPlace:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::LocationMembers:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(new_pointer, subschemas, subpair.second, walker, resolver,
                 new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::core::KeywordType::ApplicatorValueOrElements:
        [[fallthrough]];
      case sourcemeta::core::KeywordType::ApplicatorValueOrElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        } else {
          sourcemeta::core::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          walk(new_pointer, subschemas, pair.second, walker, resolver,
               new_dialect, type, level + 1);
        }

        break;
      case sourcemeta::core::KeywordType::ApplicatorElementsOrMembers:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        } else if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::core::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(new_pointer, subschemas, subpair.second, walker, resolver,
                 new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::core::KeywordType::Assertion:
        break;
      case sourcemeta::core::KeywordType::Annotation:
        break;
      case sourcemeta::core::KeywordType::Reference:
        break;
      case sourcemeta::core::KeywordType::Other:
        break;
      case sourcemeta::core::KeywordType::Comment:
        break;
      case sourcemeta::core::KeywordType::Unknown:
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

  // If the given schema declares no dialect and the user didn't
  // not pass a default, then there is nothing we can do. We know
  // the current schema is a subschema, but cannot walk any further.
  if (!dialect.has_value()) {
    this->subschemas.push_back(
        {sourcemeta::core::Pointer{}, std::nullopt, {}, std::nullopt, schema});
  } else {
    sourcemeta::core::Pointer pointer;
    walk(pointer, this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Deep, 0);
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
    walk(pointer, this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Flat, 0);
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
    this->entries.push_back(
        {{entry.first}, dialect, vocabularies, base_dialect, entry.second});
  }

  // Sort keywords based on priority for correct evaluation
  std::sort(
      this->entries.begin(), this->entries.end(),
      [&vocabularies, &walker](const auto &left, const auto &right) -> bool {
        // These cannot be empty or indexes, as we created
        // the entries array from a JSON object
        assert(!left.pointer.empty() && left.pointer.back().is_property());
        assert(!right.pointer.empty() && right.pointer.back().is_property());

        const auto left_priority = keyword_priority(
            left.pointer.back().to_property(), vocabularies, walker);
        const auto right_priority = keyword_priority(
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
