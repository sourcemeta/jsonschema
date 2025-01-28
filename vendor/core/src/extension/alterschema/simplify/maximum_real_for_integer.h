class MaximumRealForInteger final : public SchemaTransformRule {
public:
  MaximumRealForInteger()
      : SchemaTransformRule{
            "maximum_real_for_integer",
            "If an instance is guaranteed to be an integer, setting a real "
            "number upper bound is the same as a floor of that upper bound"} {};

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
                "http://json-schema.org/draft-06/schema#",
                "http://json-schema.org/draft-04/schema#",
                "http://json-schema.org/draft-03/schema#",
                "http://json-schema.org/draft-02/hyper-schema#",
                "http://json-schema.org/draft-01/hyper-schema#"}) &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "integer" &&
           schema.defines("maximum") && schema.at("maximum").is_real();
  }

  auto transform(PointerProxy &transformer) const -> void override {
    const auto current{transformer.value().at("maximum").to_real()};
    const auto new_value{static_cast<std::int64_t>(std::floor(current))};
    transformer.assign("maximum", sourcemeta::core::JSON{new_value});
  }
};
