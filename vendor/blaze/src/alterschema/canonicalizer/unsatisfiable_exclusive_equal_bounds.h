class UnsatisfiableExclusiveEqualBounds final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  UnsatisfiableExclusiveEqualBounds()
      : SchemaTransformRule{"unsatisfiable_exclusive_equal_bounds", ""} {};

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
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_3,
                                   Vocabularies::Known::JSON_Schema_Draft_4}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        (schema.at("type").to_string() == "number" ||
         schema.at("type").to_string() == "integer") &&
        schema.defines("minimum") && schema.at("minimum").is_number() &&
        schema.defines("maximum") && schema.at("maximum").is_number() &&
        schema.at("minimum") == schema.at("maximum"));

    const bool exclusive_min{schema.defines("exclusiveMinimum") &&
                             schema.at("exclusiveMinimum").is_boolean() &&
                             schema.at("exclusiveMinimum").to_boolean()};
    const bool exclusive_max{schema.defines("exclusiveMaximum") &&
                             schema.at("exclusiveMaximum").is_boolean() &&
                             schema.at("exclusiveMaximum").to_boolean()};
    ONLY_CONTINUE_IF(exclusive_min || exclusive_max);
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.into(JSON{false});
  }
};
