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
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#"}) &&
           schema.is_object() && schema.defines("not") &&
           schema.at("not").is_boolean() && !schema.at("not").to_boolean();
  }

  auto transform(JSON &schema) const -> void override { schema.erase("not"); }
};
