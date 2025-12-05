class IfWithoutThenElse final : public SchemaTransformRule {
public:
  IfWithoutThenElse()
      : SchemaTransformRule{
            "if_without_then_else",
            "The `if` keyword is meaningless "
            "without the presence of the `then` or `else` keywords"} {};

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
                     schema.is_object() && schema.defines("if") &&
                     !schema.defines("then") && !schema.defines("else"));
    return APPLIES_TO_KEYWORDS("if");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("if");
  }
};
