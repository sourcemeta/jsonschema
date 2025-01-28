class AdditionalPropertiesDefault final : public SchemaTransformRule {
public:
  AdditionalPropertiesDefault()
      : SchemaTransformRule{
            "additional_properties_default",
            "Setting the `additionalProperties` keyword to the true schema "
            "does not add any further constraint"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/applicator",
                "https://json-schema.org/draft/2019-09/vocab/applicator",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("additionalProperties") &&
           ((schema.at("additionalProperties").is_boolean() &&
             schema.at("additionalProperties").to_boolean()) ||
            (schema.at("additionalProperties").is_object() &&
             schema.at("additionalProperties").empty()));
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("additionalProperties");
  }
};
