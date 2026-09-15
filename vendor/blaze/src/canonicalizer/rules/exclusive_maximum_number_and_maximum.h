class ExclusiveMaximumNumberAndMaximum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExclusiveMaximumNumberAndMaximum()
      : SchemaTransformRule{"exclusive_maximum_number_and_maximum"} {};

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

    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number());
    const auto *exclusive_maximum{schema.try_at("exclusiveMaximum")};
    ONLY_CONTINUE_IF(exclusive_maximum && exclusive_maximum->is_number());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    if (schema.at("maximum") < schema.at("exclusiveMaximum")) {
      schema.erase("exclusiveMaximum");
    } else {
      schema.erase("maximum");
    }
  }
};
