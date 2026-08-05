#ifndef SOURCEMETA_BLAZE_CANONICALIZER_HELPERS_H_
#define SOURCEMETA_BLAZE_CANONICALIZER_HELPERS_H_

// TODO: Move upstream
inline auto IS_IN_PLACE_APPLICATOR(const SchemaKeywordType type) -> bool {
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
inline auto IS_KNOWN_TYPE_FORM(const sourcemeta::core::JSON &type,
                               const Vocabularies &vocabularies) -> bool {
  if (!vocabularies.contains_any(
          {Vocabularies::Known::JSON_Schema_Draft_0,
           Vocabularies::Known::JSON_Schema_Draft_0_Hyper,
           Vocabularies::Known::JSON_Schema_Draft_1,
           Vocabularies::Known::JSON_Schema_Draft_1_Hyper,
           Vocabularies::Known::JSON_Schema_Draft_2,
           Vocabularies::Known::JSON_Schema_Draft_2_Hyper,
           Vocabularies::Known::JSON_Schema_Draft_3,
           Vocabularies::Known::JSON_Schema_Draft_3_Hyper})) {
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

template <typename TraversePredicate, typename MatchCallback>
auto WALK_UP(const sourcemeta::core::JSON &root, const SchemaFrame &frame,
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
    const auto parent{frame.traverse(frame.uri(parent_pointer).value().get())};
    assert(parent.has_value());
    const auto parent_vocabularies{
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
auto WALK_UP_IN_PLACE_APPLICATORS(const sourcemeta::core::JSON &root,
                                  const SchemaFrame &frame,
                                  const SchemaFrame::Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver,
                                  const MatchCallback &matches)
    -> std::optional<
        std::reference_wrapper<const sourcemeta::core::WeakPointer>> {
  return WALK_UP(root, frame, location, walker, resolver,
                 IS_IN_PLACE_APPLICATOR, matches);
}

#define ONLY_CONTINUE_IF(condition)                                            \
  if (!(condition)) {                                                          \
    return false;                                                              \
  }

#endif
