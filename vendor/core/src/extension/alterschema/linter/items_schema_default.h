class ItemsSchemaDefault final : public SchemaTransformRule {
public:
  ItemsSchemaDefault()
      : SchemaTransformRule{"items_schema_default",
                            "Setting the `items` keyword to the true schema "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("items") &&
           ((schema.at("items").is_boolean() &&
             schema.at("items").to_boolean()) ||
            (schema.at("items").is_object() && schema.at("items").empty()));
  }

  auto transform(JSON &schema) const -> void override { schema.erase("items"); }
};
