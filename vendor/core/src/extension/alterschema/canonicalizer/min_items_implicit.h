class MinItemsImplicit final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinItemsImplicit()
      : SchemaTransformRule{"min_items_implicit",
                            "Every array has a minimum size of zero items"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_7,
                                   Vocabularies::Known::JSON_Schema_Draft_6,
                                   Vocabularies::Known::JSON_Schema_Draft_4,
                                   Vocabularies::Known::JSON_Schema_Draft_3,
                                   Vocabularies::Known::JSON_Schema_Draft_2,
                                   Vocabularies::Known::JSON_Schema_Draft_1}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "array" &&
        !schema.defines("minItems"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.assign("minItems", sourcemeta::core::JSON{0});
  }
};
