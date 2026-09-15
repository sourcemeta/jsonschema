class EmptyDefinitionsDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EmptyDefinitionsDrop() : SchemaTransformRule{"empty_definitions_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7}) &&
                     schema.is_object());

    const auto *definitions{schema.try_at("definitions")};
    ONLY_CONTINUE_IF(definitions && definitions->is_object() &&
                     definitions->empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("definitions");
  }
};
