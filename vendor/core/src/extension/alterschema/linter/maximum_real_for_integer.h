class MaximumRealForInteger final : public SchemaTransformRule {
public:
  MaximumRealForInteger()
      : SchemaTransformRule{
            "maximum_real_for_integer",
            "If an instance is guaranteed to be an integer, setting a real "
            "number upper bound is the same as a floor of that upper bound"} {};

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
                      "http://json-schema.org/draft-06/schema#",
                      "http://json-schema.org/draft-04/schema#",
                      "http://json-schema.org/draft-03/schema#",
                      "http://json-schema.org/draft-02/schema#",
                      "http://json-schema.org/draft-01/schema#"}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "integer" &&
        schema.defines("maximum") &&
        (schema.at("maximum").is_real() ||
         (schema.at("maximum").is_decimal() &&
          !schema.at("maximum").to_decimal().is_integer())));
    return APPLIES_TO_KEYWORDS("maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("maximum").is_decimal()) {
      auto current{schema.at("maximum").to_decimal()};
      auto new_value{current.to_integral()};
      if (new_value > current) {
        new_value -= sourcemeta::core::Decimal{1};
      }

      if (new_value.is_int64()) {
        schema.assign("maximum", sourcemeta::core::JSON{new_value.to_int64()});
      } else {
        schema.assign("maximum", sourcemeta::core::JSON{std::move(new_value)});
      }
    } else {
      const auto current{schema.at("maximum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::floor(current))};
      schema.assign("maximum", sourcemeta::core::JSON{new_value});
    }
  }
};
