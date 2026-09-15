class ConstAsEnum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ConstAsEnum() : SchemaTransformRule{"const_as_enum"} {};

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
        schema.is_object() && schema.defines("const") &&
        !schema.defines("enum"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    auto values = sourcemeta::core::JSON::make_array();
    values.push_back(schema.at("const"));
    schema.at("const").into(std::move(values));
    schema.rename("const", "enum");
  }
};
