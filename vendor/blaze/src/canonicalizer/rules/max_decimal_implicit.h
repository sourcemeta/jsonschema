class MaxDecimalImplicit final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  MaxDecimalImplicit() : SchemaTransformRule{"max_decimal_implicit"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1}) &&
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
