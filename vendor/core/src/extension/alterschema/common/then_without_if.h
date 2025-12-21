class ThenWithoutIf final : public SchemaTransformRule {
public:
  ThenWithoutIf()
      : SchemaTransformRule{"then_without_if",
                            "The `then` keyword is meaningless "
                            "without the presence of the `if` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object() && schema.defines("then") &&
                     !schema.defines("if"));
    return APPLIES_TO_KEYWORDS("then");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("then");
  }
};
