class ThenEmpty final : public SchemaTransformRule {
public:
  ThenEmpty()
      : SchemaTransformRule{"then_empty",
                            "Setting the `then` keyword to the empty schema "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#"}) &&
           schema.is_object() && schema.defines("then") &&
           is_schema(schema.at("then")) && is_empty_schema(schema.at("then")) &&
           (schema.at("then").is_object() ||
            (!schema.defines("if") ||
             !(schema.at("if").is_boolean() && schema.at("if").to_boolean())));
  }

  auto transform(JSON &schema) const -> void override { schema.erase("then"); }
};
