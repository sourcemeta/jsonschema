class UnsatisfiableCanEqualBounds final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  UnsatisfiableCanEqualBounds()
      : SchemaTransformRule{"unsatisfiable_can_equal_bounds", ""} {};

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
        vocabularies.contains(Vocabularies::Known::JSON_Schema_Draft_2) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        (schema.at("type").to_string() == "number" ||
         schema.at("type").to_string() == "integer") &&
        schema.defines("minimum") && schema.at("minimum").is_number() &&
        schema.defines("maximum") && schema.at("maximum").is_number() &&
        schema.at("minimum") == schema.at("maximum"));

    const bool min_exclusive{schema.defines("minimumCanEqual") &&
                             schema.at("minimumCanEqual").is_boolean() &&
                             !schema.at("minimumCanEqual").to_boolean()};
    const bool max_exclusive{schema.defines("maximumCanEqual") &&
                             schema.at("maximumCanEqual").is_boolean() &&
                             !schema.at("maximumCanEqual").to_boolean()};
    ONLY_CONTINUE_IF(min_exclusive || max_exclusive);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{false});
  }
};
