class EnumToConst final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EnumToConst() : SchemaTransformRule{"enum_to_const"} {};

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
        schema.is_object() && !schema.defines("const"));

    const auto *enum_value{schema.try_at("enum")};
    ONLY_CONTINUE_IF(enum_value && enum_value->is_array() &&
                     enum_value->size() == 1);
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto front{schema.at("enum").front()};
    schema.at("enum").into(front);
    schema.rename("enum", "const");
  }
};
