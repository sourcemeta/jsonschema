class UnevaluatedItemsDefault final : public SchemaTransformRule {
public:
  UnevaluatedItemsDefault()
      : SchemaTransformRule{
            "unevaluated_items_default",
            "Setting the `unevaluatedItems` keyword to the true schema "
            "does not add any further constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
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

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("unevaluatedItems");
  }
};
