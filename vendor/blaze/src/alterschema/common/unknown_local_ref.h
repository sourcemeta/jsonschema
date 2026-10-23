class UnknownLocalRef final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"$ref"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  UnknownLocalRef()
      : SchemaTransformRule{
            "unknown_local_ref",
            "Local references that point to unknown locations are invalid and "
            "will result in evaluation failures"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &,
            const SchemaVocabularies &vocabularies, const SchemaFrame &frame,
            const SchemaFrame::Location &location, const SchemaWalker &,
            const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE,
         // In JSON Schema Draft 7 and older, `$ref` overrides siblings.
         // However, we do not need to worry about this case here, as if the
         // `$ref` points to an unknown local location, the entire schema is
         // invalid anyway. We just help at least making the schema valid
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::OPENAPI_3_0_BASE}));
    ONLY_CONTINUE_IF(schema.is_object() && schema.defines(KEYWORD) &&
                     schema.at(KEYWORD).is_string());

    // Find the keyword location entry
    auto keyword_pointer{location.pointer};
    keyword_pointer.push_back(std::cref(KEYWORD));
    const auto reference_entry{
        frame.reference(SchemaReferenceType::Static, keyword_pointer)};
    ONLY_CONTINUE_IF(reference_entry.has_value());

    // Only continue if the reference target does not exist
    ONLY_CONTINUE_IF(
        !frame.traverse(reference_entry->get().destination).has_value());

    // Where the reference leads has to be this document for it to be local at
    // all. An empty base is a destination that names no document of its own,
    // which is how a reference spelled as a fragment alone or as nothing at
    // all resolves in a schema that has no base URI, so it never left.
    // Otherwise the base is either the one this schema already resolves
    // against, or one that names a resource the document holds, such as an
    // embedded identifier.
    //
    // A wrapper format puts the schemas it holds below a root that is no
    // schema resource of its own, so nothing framed ever answers to the
    // document's own base. A reference to that base never left the document
    // all the same, which is why it is compared before the frame is asked. A
    // reference spelled without a fragment reaches here for that case alone,
    // as its destination is its base, and a base the frame knows would have
    // been found as the destination just above
    const auto &reference_base{reference_entry->get().base};
    ONLY_CONTINUE_IF(reference_base.empty() ||
                     reference_base == location.base ||
                     frame.traverse(reference_base).has_value());

    return applies_to_keywords(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
