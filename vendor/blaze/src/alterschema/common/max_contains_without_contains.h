class MaxContainsWithoutContains final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MaxContainsWithoutContains()
      : SchemaTransformRule{"max_contains_without_contains",
                            "The `maxContains` keyword is meaningless "
                            "without the presence of the `contains` keyword"} {
        };

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION}) &&
        schema.is_object() && schema.defines("maxContains") &&
        !schema.defines("contains"));
    return applies_to_keywords("maxContains");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("maxContains");
  }
};
