class SingleTypeArray final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  SingleTypeArray() : SchemaTransformRule{"single_type_array"} {};

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
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_array() && type->size() == 1 &&
                     type->front().is_string());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto type{schema.at("type").front()};
    schema.at("type").into(std::move(type));
  }
};
