class EnumWithType final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EnumWithType() : SchemaTransformRule{"enum_with_type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type);
    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array());

    ONLY_CONTINUE_IF(type_is_implied_by_enum(*type, *enum_value, vocabularies));

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("type");
  }
};
