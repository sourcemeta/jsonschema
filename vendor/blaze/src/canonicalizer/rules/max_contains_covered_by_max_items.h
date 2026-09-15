class MaxContainsCoveredByMaxItems final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  MaxContainsCoveredByMaxItems()
      : SchemaTransformRule{"max_contains_covered_by_max_items"} {};

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
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION}) &&
        schema.is_object());

    const auto *max_contains{schema.try_at("maxContains")};
    ONLY_CONTINUE_IF(max_contains && max_contains->is_integer());
    const auto *max_items{schema.try_at("maxItems")};
    ONLY_CONTINUE_IF(max_items && max_items->is_integer() &&
                     max_contains->to_integer() > max_items->to_integer());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("maxContains", schema.at("maxItems"));
  }
};
