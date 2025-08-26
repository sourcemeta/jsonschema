class MinContainsWithoutContains final : public SchemaTransformRule {
public:
  MinContainsWithoutContains()
      : SchemaTransformRule{"min_contains_without_contains",
                            "The `minContains` keyword is meaningless "
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
        contains_any(
            vocabularies,
            {"https://json-schema.org/draft/2020-12/vocab/validation",
             "https://json-schema.org/draft/2019-09/vocab/validation"}) &&
        schema.is_object() && schema.defines("minContains") &&
        !schema.defines("contains"));
    return APPLIES_TO_KEYWORDS("minContains");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("minContains");
  }
};
