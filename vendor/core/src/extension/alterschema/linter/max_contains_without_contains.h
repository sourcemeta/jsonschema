class MaxContainsWithoutContains final : public SchemaTransformRule {
public:
  MaxContainsWithoutContains()
      : SchemaTransformRule{"max_contains_without_contains",
                            "The `maxContains` keyword is meaningless "
                            "without the presence of the `contains` keyword"} {
        };

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
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Validation,
             Vocabularies::Known::JSON_Schema_2019_09_Validation}) &&
        schema.is_object() && schema.defines("maxContains") &&
        !schema.defines("contains"));
    return APPLIES_TO_KEYWORDS("maxContains");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("maxContains");
  }
};
