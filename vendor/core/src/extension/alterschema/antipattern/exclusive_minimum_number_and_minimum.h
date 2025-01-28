class ExclusiveMinimumNumberAndMinimum final : public SchemaTransformRule {
public:
  ExclusiveMinimumNumberAndMinimum()
      : SchemaTransformRule{
            "exclusive_minimum_number_and_minimum",
            "Setting both `exclusiveMinimum` and `minimum` at the same time "
            "is considered an anti-pattern. You should choose one"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/validation",
                "https://json-schema.org/draft/2019-09/vocab/validation",
                "http://json-schema.org/draft-07/schema#",
                "http://json-schema.org/draft-06/schema#"}) &&
           schema.is_object() && schema.defines("minimum") &&
           schema.defines("exclusiveMinimum") &&
           schema.at("minimum").is_number() &&
           schema.at("exclusiveMinimum").is_number();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    if (transformer.value().at("exclusiveMinimum") <
        transformer.value().at("minimum")) {
      transformer.erase("exclusiveMinimum");
    } else {
      transformer.erase("minimum");
    }
  }
};
