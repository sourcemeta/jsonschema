class NotFalse final : public SchemaTransformRule {
public:
  NotFalse()
      : SchemaTransformRule{"not_false",
                            "Setting the `not` keyword to `false` imposes no "
                            "constraints. Negating `false` yields the "
                            "always-true schema"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("not") &&
                     schema.at("not").is_boolean() &&
                     !schema.at("not").to_boolean());
    return APPLIES_TO_KEYWORDS("not");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("not");
  }
};
