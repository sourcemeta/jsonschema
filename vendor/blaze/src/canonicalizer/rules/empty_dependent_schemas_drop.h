class EmptyDependentSchemasDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EmptyDependentSchemasDrop()
      : SchemaTransformRule{"empty_dependent_schemas_drop"} {};

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
            {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_APPLICATOR,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_APPLICATOR}) &&
        schema.is_object());

    const auto *dependent_schemas{schema.try_at("dependentSchemas")};
    ONLY_CONTINUE_IF(dependent_schemas && dependent_schemas->is_object() &&
                     dependent_schemas->empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("dependentSchemas");
  }
};
