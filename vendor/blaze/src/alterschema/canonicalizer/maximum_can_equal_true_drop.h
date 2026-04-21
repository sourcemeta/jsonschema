class MaximumCanEqualTrueDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MaximumCanEqualTrueDrop()
      : SchemaTransformRule{"maximum_can_equal_true_drop", ""} {};

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
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        (schema.at("type").to_string() == "integer" ||
         schema.at("type").to_string() == "number") &&
        schema.defines("maximumCanEqual") &&
        schema.at("maximumCanEqual").is_boolean() &&
        schema.at("maximumCanEqual").to_boolean());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("maximumCanEqual");
  }
};
