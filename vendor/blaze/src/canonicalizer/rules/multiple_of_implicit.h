class MultipleOfImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MultipleOfImplicit() : SchemaTransformRule{"multiple_of_implicit"} {};

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
        schema.is_object() && !schema.defines("multipleOf"));

    const auto *type{schema.try_at("type")};
    // Applying this to numbers would be a semantic problem
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("multipleOf", sourcemeta::core::JSON{1});
  }
};
