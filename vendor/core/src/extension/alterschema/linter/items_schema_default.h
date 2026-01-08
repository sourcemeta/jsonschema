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
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator,
             Vocabularies::Known::JSON_Schema_Draft_7,
             Vocabularies::Known::JSON_Schema_Draft_6,
             Vocabularies::Known::JSON_Schema_Draft_4,
             Vocabularies::Known::JSON_Schema_Draft_3,
             Vocabularies::Known::JSON_Schema_Draft_2,
             Vocabularies::Known::JSON_Schema_Draft_2_Hyper,
             Vocabularies::Known::JSON_Schema_Draft_1,
             Vocabularies::Known::JSON_Schema_Draft_1_Hyper}) &&
        schema.is_object() && schema.defines("items") &&
        ((schema.at("items").is_boolean() && schema.at("items").to_boolean()) ||
         (schema.at("items").is_object() && schema.at("items").empty())));
    ONLY_CONTINUE_IF(
        !frame.has_references_through(location.pointer.concat({"items"})));
    return APPLIES_TO_KEYWORDS("items");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("items");
  }
};
