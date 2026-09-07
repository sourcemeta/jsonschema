class NotFalse final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"not"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NotFalse()
      : SchemaTransformRule{"not_false",
                            "Setting the `not` keyword to `false` imposes no "
                            "constraints. Negating `false` yields the "
                            "always-true schema"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &,
            const SchemaVocabularies &vocabularies, const SchemaFrame &frame,
            const SchemaFrame::Location &location, const SchemaWalker &,
            const SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4}) &&
        schema.is_object() && schema.defines(KEYWORD) &&
        schema.at(KEYWORD).is_boolean() && !schema.at(KEYWORD).to_boolean());
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return applies_to_keywords(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
