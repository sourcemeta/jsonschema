class ElseEmpty final : public SchemaTransformRule {
public:
  ElseEmpty()
      : SchemaTransformRule{"else_empty",
                            "Setting the `else` keyword to the empty schema "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const JSON &schema, const JSON &, const Vocabularies &vocabularies,
            const SchemaFrame &, const SchemaFrame::Location &,
            const SchemaWalker &, const SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_Draft_7}) &&
        schema.is_object() && schema.defines("else") &&
        is_schema(schema.at("else")) && is_empty_schema(schema.at("else")) &&
        (schema.at("else").is_object() ||
         (!schema.defines("if") ||
          !(schema.at("if").is_boolean() && schema.at("if").to_boolean()))));
    return APPLIES_TO_KEYWORDS("else");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("else");
  }
};
