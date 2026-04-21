class MaxDecimalImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MaxDecimalImplicit() : SchemaTransformRule{"max_decimal_implicit", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "integer" &&
        !schema.defines("maxDecimal"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("maxDecimal", sourcemeta::core::JSON{0});
  }
};
