#ifndef SOURCEMETA_BLAZE_CANONICALIZER_HELPERS_H_
#define SOURCEMETA_BLAZE_CANONICALIZER_HELPERS_H_

// TODO: Move upstream
inline auto is_in_place_applicator(const SchemaKeywordType type) -> bool {
  return type == SchemaKeywordType::ApplicatorValueOrElementsInPlace ||
         type == SchemaKeywordType::ApplicatorMembersInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlace ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSome ||
         type == SchemaKeywordType::ApplicatorElementsInPlaceSomeNegate ||
         type == SchemaKeywordType::ApplicatorValueInPlaceMaybe ||
         type == SchemaKeywordType::ApplicatorValueInPlaceOther ||
         type == SchemaKeywordType::ApplicatorValueInPlaceNegate;
}

// Walk up from a schema location, continuing as long as the traversal
// predicate returns true for each keyword type encountered. Returns a
// reference to the pointer of the ancestor where the match callback returned
// true, or nullopt if no match was found or the traversal predicate stopped
// the walk.
// Whether a `type` value only consists of simple type names that can be
// parsed into a complete set of instance types. Draft 0 to Draft 3 unions
// may contain subschemas or `any`, in which case the parsed set is an
// under-approximation that cannot be trusted. Later dialects do not give
// such forms any meaning, so the parsed set stands
inline auto is_known_type_form(const sourcemeta::core::JSON &type,
                               const SchemaVocabularies &vocabularies) -> bool {
  if (!vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
    return true;
  }
  if (type.is_string()) {
    return type.to_string() != "any";
  }
  if (!type.is_array()) {
    return false;
  }
  return std::ranges::all_of(type.as_array(), [](const auto &entry) -> auto {
    return entry.is_string() && entry.to_string() != "any";
  });
}

// The schema that no instance can ever satisfy. Draft 3 and Draft 4 have no
// boolean schemas, so each of them has to spell it out with the negation
// keyword it offers, applied to the schema that every instance satisfies
inline auto unsatisfiable_schema(const SchemaVocabularies &vocabularies)
    -> sourcemeta::core::JSON {
  if (vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4_HYPER})) {
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("not", sourcemeta::core::JSON::make_object());
    return result;
  }

  if (vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER})) {
    auto branches{sourcemeta::core::JSON::make_array()};
    branches.push_back(sourcemeta::core::JSON::make_object());
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("disallow", std::move(branches));
    return result;
  }

  // Draft 2 and earlier have no boolean schemas either, and their negation
  // keyword takes type names rather than schemas, so the name to rule out is
  // the wildcard that every instance answers to
  if (vocabularies.contains_any(
          {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1_HYPER,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
           SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2_HYPER})) {
    auto types{sourcemeta::core::JSON::make_array()};
    types.push_back(sourcemeta::core::JSON{"any"});
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("disallow", std::move(types));
    return result;
  }

  return sourcemeta::core::JSON{false};
}

// Collapsing a schema to the unsatisfiable one throws away every keyword it
// had, `$schema` included. A boolean cannot carry the dialect back, so the
// caller is left to reset the frame, but an object both can and must. Rewrite
// it in place rather than replacing it outright, so that the dialect the frame
// still points to stays where it is
inline auto into_unsatisfiable(sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &unsatisfiable)
    -> void {
  if (!unsatisfiable.is_object() || !schema.is_object()) {
    schema.into(sourcemeta::core::JSON{unsatisfiable});
    return;
  }

  std::vector<sourcemeta::core::JSON::String> superseded;
  for (const auto &entry : schema.as_object()) {
    if (entry.first != "$schema") {
      superseded.push_back(entry.first);
    }
  }

  for (const auto &keyword : superseded) {
    schema.erase(keyword);
  }

  for (const auto &entry : unsatisfiable.as_object()) {
    schema.assign(entry.first, entry.second);
  }
}

template <typename TraversePredicate, typename MatchCallback>
auto walk_up(const sourcemeta::core::JSON &root, const SchemaFrame &frame,
             const SchemaFrame::Location &location, const SchemaWalker &walker,
             const SchemaResolver &resolver,
             const TraversePredicate &should_continue,
             const MatchCallback &matches)
    -> std::optional<
        std::reference_wrapper<const sourcemeta::core::WeakPointer>> {
  auto current_pointer{location.pointer};
  auto current_parent{location.parent};

  while (current_parent.has_value()) {
    const auto &parent_pointer{current_parent.value()};
    const auto relative_pointer{current_pointer.resolve_from(parent_pointer)};
    assert(!relative_pointer.empty() && relative_pointer.at(0).is_property());
    const auto parent{frame.traverse(parent_pointer)};
    assert(parent.has_value());
    const auto &parent_vocabularies{
        frame.vocabularies(parent.value().get(), resolver)};
    const auto keyword_type{
        walker(relative_pointer.at(0).to_property(), parent_vocabularies).type};

    if (!should_continue(keyword_type)) {
      return std::nullopt;
    }

    if (matches(sourcemeta::core::get(root, parent_pointer),
                parent_vocabularies)) {
      return std::cref(parent.value().get().pointer);
    }

    current_pointer = parent_pointer;
    current_parent = parent.value().get().parent;
  }

  return std::nullopt;
}

template <typename MatchCallback>
auto walk_up_in_place_applicators(const sourcemeta::core::JSON &root,
                                  const SchemaFrame &frame,
                                  const SchemaFrame::Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver,
                                  const MatchCallback &matches)
    -> std::optional<
        std::reference_wrapper<const sourcemeta::core::WeakPointer>> {
  return walk_up(root, frame, location, walker, resolver,
                 is_in_place_applicator, matches);
}

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

#endif
