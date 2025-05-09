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
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/unevaluated",
                "https://json-schema.org/draft/2019-09/vocab/applicator"}) &&
           schema.is_object() && schema.defines("unevaluatedItems") &&
           ((schema.at("unevaluatedItems").is_boolean() &&
             schema.at("unevaluatedItems").to_boolean()) ||
            (schema.at("unevaluatedItems").is_object() &&
             schema.at("unevaluatedItems").empty()));
  }

  auto transform(JSON &schema) const -> void override {
    schema.erase("unevaluatedItems");
  }
};
