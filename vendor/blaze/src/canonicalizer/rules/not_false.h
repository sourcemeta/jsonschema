class NotFalse final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"not"};

public:
  using reframe_after_transform = std::true_type;
  NotFalse() : SchemaTransformRule{"not_false"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const sourcemeta::core::JSON &,
                               const SchemaVocabularies &vocabularies,
                               const SchemaFrame &frame,
                               const SchemaFrame::Location &location,
                               const SchemaWalker &,
                               const SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4}) &&
        schema.is_object() && schema.defines(KEYWORD) &&
        schema.at(KEYWORD).is_boolean() && !schema.at(KEYWORD).to_boolean());
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase(KEYWORD);
  }
};
