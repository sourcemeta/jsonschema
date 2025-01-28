#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <algorithm> // std::max, std::sort
#include <cassert>   // assert
#include <numeric>   // std::accumulate

auto sourcemeta::jsontoolkit::keyword_priority(
    std::string_view keyword, const std::map<std::string, bool> &vocabularies,
    const sourcemeta::jsontoolkit::SchemaWalker &walker) -> std::uint64_t {
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

auto walk(sourcemeta::jsontoolkit::Pointer &pointer,
          std::vector<sourcemeta::jsontoolkit::SchemaIteratorEntry> &subschemas,
          const sourcemeta::jsontoolkit::JSON &subschema,
          const sourcemeta::jsontoolkit::SchemaWalker &walker,
          const sourcemeta::jsontoolkit::SchemaResolver &resolver,
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
      sourcemeta::jsontoolkit::dialect(subschema, dialect)};
  assert(current_dialect.has_value());
  const std::string &new_dialect{current_dialect.value()};

  const std::optional<std::string> base_dialect{
      sourcemeta::jsontoolkit::base_dialect(subschema, resolver, new_dialect)};
  assert(base_dialect.has_value());
  const std::map<std::string, bool> vocabularies{
      sourcemeta::jsontoolkit::vocabularies(resolver, base_dialect.value(),
                                            new_dialect)};

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
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorValue:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorValueOther:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorValueInPlace: {
        sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
        new_pointer.emplace_back(pair.first);
        walk(new_pointer, subschemas, pair.second, walker, resolver,
             new_dialect, type, level + 1);
      } break;
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorElements:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorElementsInline:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorMembers:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorMembersInPlace:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::LocationMembers:
        if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(new_pointer, subschemas, subpair.second, walker, resolver,
                 new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorValueOrElements:
        [[fallthrough]];
      case sourcemeta::jsontoolkit::KeywordType::
          ApplicatorValueOrElementsInPlace:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        } else {
          sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
          new_pointer.emplace_back(pair.first);
          walk(new_pointer, subschemas, pair.second, walker, resolver,
               new_dialect, type, level + 1);
        }

        break;
      case sourcemeta::jsontoolkit::KeywordType::ApplicatorElementsOrMembers:
        if (pair.second.is_array()) {
          for (std::size_t index = 0; index < pair.second.size(); index++) {
            sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(index);
            walk(new_pointer, subschemas, pair.second.at(index), walker,
                 resolver, new_dialect, type, level + 1);
          }
        } else if (pair.second.is_object()) {
          for (auto &subpair : pair.second.as_object()) {
            sourcemeta::jsontoolkit::Pointer new_pointer{pointer};
            new_pointer.emplace_back(pair.first);
            new_pointer.emplace_back(subpair.first);
            walk(new_pointer, subschemas, subpair.second, walker, resolver,
                 new_dialect, type, level + 1);
          }
        }

        break;
      case sourcemeta::jsontoolkit::KeywordType::Assertion:
        break;
      case sourcemeta::jsontoolkit::KeywordType::Annotation:
        break;
      case sourcemeta::jsontoolkit::KeywordType::Reference:
        break;
      case sourcemeta::jsontoolkit::KeywordType::Other:
        break;
      case sourcemeta::jsontoolkit::KeywordType::Comment:
        break;
      case sourcemeta::jsontoolkit::KeywordType::Unknown:
        break;
    }
  }
}
} // namespace

// TODO: These iterators are not very efficient. They traverse once on
// construction and then the client traverses again.

sourcemeta::jsontoolkit::SchemaIterator::SchemaIterator(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};

  // If the given schema declares no dialect and the user didn't
  // not pass a default, then there is nothing we can do. We know
  // the current schema is a subschema, but cannot walk any further.
  if (!dialect.has_value()) {
    this->subschemas.push_back({sourcemeta::jsontoolkit::Pointer{},
                                std::nullopt,
                                {},
                                std::nullopt,
                                schema});
  } else {
    sourcemeta::jsontoolkit::Pointer pointer;
    walk(pointer, this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Deep, 0);
  }
}

sourcemeta::jsontoolkit::SchemaIteratorFlat::SchemaIteratorFlat(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  if (dialect.has_value()) {
    sourcemeta::jsontoolkit::Pointer pointer;
    walk(pointer, this->subschemas, schema, walker, resolver, dialect.value(),
         SchemaWalkerType_t::Flat, 0);
  }
}

sourcemeta::jsontoolkit::SchemaKeywordIterator::SchemaKeywordIterator(
    const sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect) {
  assert(is_schema(schema));
  if (schema.is_boolean()) {
    return;
  }

  const std::optional<std::string> dialect{
      sourcemeta::jsontoolkit::dialect(schema, default_dialect)};
  const std::optional<std::string> base_dialect{
      sourcemeta::jsontoolkit::base_dialect(schema, resolver, dialect)};

  std::map<std::string, bool> vocabularies;
  if (base_dialect.has_value() && dialect.has_value()) {
    vocabularies.merge(sourcemeta::jsontoolkit::vocabularies(
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

auto sourcemeta::jsontoolkit::SchemaIterator::begin() const -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::jsontoolkit::SchemaIterator::end() const -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::jsontoolkit::SchemaIterator::cbegin() const -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::jsontoolkit::SchemaIterator::cend() const -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::jsontoolkit::SchemaIteratorFlat::begin() const
    -> const_iterator {
  return this->subschemas.begin();
}
auto sourcemeta::jsontoolkit::SchemaIteratorFlat::end() const
    -> const_iterator {
  return this->subschemas.end();
}
auto sourcemeta::jsontoolkit::SchemaIteratorFlat::cbegin() const
    -> const_iterator {
  return this->subschemas.cbegin();
}
auto sourcemeta::jsontoolkit::SchemaIteratorFlat::cend() const
    -> const_iterator {
  return this->subschemas.cend();
}

auto sourcemeta::jsontoolkit::SchemaKeywordIterator::begin() const
    -> const_iterator {
  return this->entries.begin();
}
auto sourcemeta::jsontoolkit::SchemaKeywordIterator::end() const
    -> const_iterator {
  return this->entries.end();
}
auto sourcemeta::jsontoolkit::SchemaKeywordIterator::cbegin() const
    -> const_iterator {
  return this->entries.cbegin();
}
auto sourcemeta::jsontoolkit::SchemaKeywordIterator::cend() const
    -> const_iterator {
  return this->entries.cend();
}
