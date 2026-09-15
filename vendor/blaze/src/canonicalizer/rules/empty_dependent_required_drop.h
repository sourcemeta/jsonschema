class EmptyDependentRequiredDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EmptyDependentRequiredDrop()
      : SchemaTransformRule{"empty_dependent_required_drop"} {};

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
            {SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION}) &&
        schema.is_object());

    const auto *dependent_required{schema.try_at("dependentRequired")};
    ONLY_CONTINUE_IF(dependent_required && dependent_required->is_object() &&
                     dependent_required->empty());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("dependentRequired");
  }
};
