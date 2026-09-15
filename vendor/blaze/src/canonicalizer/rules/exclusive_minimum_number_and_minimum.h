class ExclusiveMinimumNumberAndMinimum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExclusiveMinimumNumberAndMinimum()
      : SchemaTransformRule{"exclusive_minimum_number_and_minimum"} {};

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
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6}) &&
        schema.is_object());

    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number());
    const auto *exclusive_minimum{schema.try_at("exclusiveMinimum")};
    ONLY_CONTINUE_IF(exclusive_minimum && exclusive_minimum->is_number());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (schema.at("exclusiveMinimum") < schema.at("minimum")) {
      schema.erase("exclusiveMinimum");
    } else {
      schema.erase("minimum");
    }
  }
};
