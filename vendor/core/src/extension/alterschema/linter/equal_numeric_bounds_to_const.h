class EqualNumericBoundsToConst final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EqualNumericBoundsToConst()
      : SchemaTransformRule{
            "equal_numeric_bounds_to_const",
            "Setting `minimum` and `maximum` to the same number only leaves "
            "one possible value"} {};

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
        vocabularies.contains_any({
            Vocabularies::Known::JSON_Schema_2020_12_Validation,
            Vocabularies::Known::JSON_Schema_2019_09_Validation,
            Vocabularies::Known::JSON_Schema_Draft_7,
            Vocabularies::Known::JSON_Schema_Draft_6,
        }) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        (schema.at("type").to_string() == "integer" ||
         schema.at("type").to_string() == "number") &&
        schema.defines("minimum") && schema.at("minimum").is_number() &&
        schema.defines("maximum") && schema.at("maximum").is_number() &&
        schema.at("minimum") == schema.at("maximum"));
    return APPLIES_TO_KEYWORDS("minimum", "maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("minimum", "const");
    schema.erase("type");
    schema.erase("maximum");
  }
};
