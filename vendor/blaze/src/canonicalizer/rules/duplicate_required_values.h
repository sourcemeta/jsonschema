class DuplicateRequiredValues final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  DuplicateRequiredValues()
      : SchemaTransformRule{"duplicate_required_values"} {};

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
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4}) &&
        schema.is_object());

    const auto *required{schema.try_at("required")};
    ONLY_CONTINUE_IF(required && required->is_array() && !required->unique());
    // TODO: Highlight which specific entries in `required` are duplicated
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto collection = schema.at("required");
    std::sort(collection.as_array().begin(), collection.as_array().end());
    auto last =
        std::unique(collection.as_array().begin(), collection.as_array().end());
    collection.erase(last, collection.as_array().end());
    schema.at("required").into(std::move(collection));
  }
};
