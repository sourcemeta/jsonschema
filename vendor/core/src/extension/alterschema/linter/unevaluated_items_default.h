class UnevaluatedItemsDefault final : public SchemaTransformRule {
public:
  UnevaluatedItemsDefault()
      : SchemaTransformRule{
            "unevaluated_items_default",
            "Setting the `unevaluatedItems` keyword to the true schema "
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
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Unevaluated,
             Vocabularies::Known::JSON_Schema_2019_09_Applicator}) &&
        schema.is_object() && schema.defines("unevaluatedItems") &&
        ((schema.at("unevaluatedItems").is_boolean() &&
          schema.at("unevaluatedItems").to_boolean()) ||
         (schema.at("unevaluatedItems").is_object() &&
          schema.at("unevaluatedItems").empty())));
    return APPLIES_TO_KEYWORDS("unevaluatedItems");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("unevaluatedItems");
  }
};
