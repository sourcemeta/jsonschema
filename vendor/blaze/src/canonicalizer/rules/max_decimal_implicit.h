class MaxDecimalImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MaxDecimalImplicit() : SchemaTransformRule{"max_decimal_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_0,
                          SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}) &&
                     schema.is_object() && !schema.defines("maxDecimal"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.assign("maxDecimal", sourcemeta::core::JSON{0});
  }
};
