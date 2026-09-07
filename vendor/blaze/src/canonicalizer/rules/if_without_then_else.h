class IfWithoutThenElse final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(cert-err58-cpp,bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"if"};

public:
  using reframe_after_transform = std::true_type;
  IfWithoutThenElse() : SchemaTransformRule{"if_without_then_else"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7}) &&
        schema.is_object() && schema.defines(KEYWORD) &&
        !schema.defines("then") && !schema.defines("else"));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase(KEYWORD);
  }
};
