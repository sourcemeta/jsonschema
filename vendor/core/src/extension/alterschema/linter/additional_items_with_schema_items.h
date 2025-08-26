class AdditionalItemsWithSchemaItems final : public SchemaTransformRule {
public:
  AdditionalItemsWithSchemaItems()
      : SchemaTransformRule{"additional_items_with_schema_items",
                            "The `additionalItems` keyword is ignored when the "
                            "`items` keyword is set to a schema"} {};

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
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2019-09/vocab/applicator",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#"}) &&
        schema.is_object() && schema.defines("items") &&
        schema.defines("additionalItems") && is_schema(schema.at("items")));
    return APPLIES_TO_KEYWORDS("additionalItems", "items");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("additionalItems");
  }
};
