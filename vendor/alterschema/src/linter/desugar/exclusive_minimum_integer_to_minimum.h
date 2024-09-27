class ExclusiveMinimumIntegerToMinimum final : public Rule {
public:
  ExclusiveMinimumIntegerToMinimum()
      : Rule("exclusive_minimum_integer_to_minimum",
             "Setting `exclusiveMinimum` when `type` is `integer` is syntax "
             "sugar for `minimum`") {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
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

  auto transform(Transformer &transformer) const -> void override {
    if (transformer.schema().at("exclusiveMinimum").is_integer()) {
      auto new_minimum = transformer.schema().at("exclusiveMinimum");
      new_minimum += sourcemeta::jsontoolkit::JSON{1};
      transformer.assign("minimum", new_minimum);
      transformer.erase("exclusiveMinimum");
    } else {
      const auto current{transformer.schema().at("exclusiveMinimum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::ceil(current))};
      transformer.assign("minimum", sourcemeta::jsontoolkit::JSON{new_value});
      transformer.erase("exclusiveMinimum");
    }
  }
};
