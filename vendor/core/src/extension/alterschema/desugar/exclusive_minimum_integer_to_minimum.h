class ExclusiveMinimumIntegerToMinimum final : public SchemaTransformRule {
public:
  ExclusiveMinimumIntegerToMinimum()
      : SchemaTransformRule{
            "exclusive_minimum_integer_to_minimum",
            "Setting `exclusiveMinimum` when `type` is `integer` is syntax "
            "sugar for `minimum`"} {};

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
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "integer" &&
           schema.defines("exclusiveMinimum") &&
           schema.at("exclusiveMinimum").is_number() &&
           !schema.defines("minimum");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    if (transformer.value().at("exclusiveMinimum").is_integer()) {
      auto new_minimum = transformer.value().at("exclusiveMinimum");
      new_minimum += sourcemeta::core::JSON{1};
      transformer.assign("minimum", new_minimum);
      transformer.erase("exclusiveMinimum");
    } else {
      const auto current{transformer.value().at("exclusiveMinimum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::ceil(current))};
      transformer.assign("minimum", sourcemeta::core::JSON{new_value});
      transformer.erase("exclusiveMinimum");
    }
  }
};
