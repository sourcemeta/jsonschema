class CommentDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  CommentDrop() : SchemaTransformRule{"comment_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE}) &&
        schema.is_object() && schema.defines("$comment"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("$comment");
  }
};
