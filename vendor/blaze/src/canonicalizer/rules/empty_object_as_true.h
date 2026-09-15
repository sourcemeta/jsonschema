class EmptyObjectAsTrue final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  EmptyObjectAsTrue() : SchemaTransformRule{"empty_object_as_true"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_CORE,
                          SchemaVocabularies::Known::JSON_SCHEMA_2019_09_CORE,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6}) &&
                     schema.is_object() && schema.empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.into(sourcemeta::core::JSON{true});
  }
};
