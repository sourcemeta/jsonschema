class ExclusiveMinimumIntegerToMinimum final : public SchemaTransformRule {
public:
  ExclusiveMinimumIntegerToMinimum()
      : SchemaTransformRule{
            "exclusive_minimum_integer_to_minimum",
            "Setting `exclusiveMinimum` when `type` is `integer` is syntax "
            "sugar for `minimum`"} {};

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
                     {"https://json-schema.org/draft/2020-12/vocab/validation",
                      "https://json-schema.org/draft/2019-09/vocab/validation",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#"}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "integer" &&
        schema.defines("exclusiveMinimum") &&
        schema.at("exclusiveMinimum").is_number() &&
        !schema.defines("minimum"));
    return APPLIES_TO_KEYWORDS("exclusiveMinimum", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("exclusiveMinimum").is_integer()) {
      auto new_minimum = schema.at("exclusiveMinimum");
      new_minimum += sourcemeta::core::JSON{1};
      schema.at("exclusiveMinimum").into(new_minimum);
      schema.rename("exclusiveMinimum", "minimum");
    } else if (schema.at("exclusiveMinimum").is_decimal()) {
      const auto current{schema.at("exclusiveMinimum").to_decimal()};
      auto new_value{current.to_integral()};
      if (new_value < current) {
        new_value += sourcemeta::core::Decimal{1};
      }

      if (current.is_integer()) {
        new_value += sourcemeta::core::Decimal{1};
      }

      if (new_value.is_int64()) {
        schema.at("exclusiveMinimum")
            .into(sourcemeta::core::JSON{new_value.to_int64()});
      } else {
        schema.at("exclusiveMinimum").into(sourcemeta::core::JSON{new_value});
      }

      schema.rename("exclusiveMinimum", "minimum");
    } else {
      const auto current{schema.at("exclusiveMinimum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::ceil(current))};
      schema.at("exclusiveMinimum").into(sourcemeta::core::JSON{new_value});
      schema.rename("exclusiveMinimum", "minimum");
    }
  }
};
