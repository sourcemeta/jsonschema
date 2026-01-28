class ExclusiveMaximumIntegerToMaximum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExclusiveMaximumIntegerToMaximum()
      : SchemaTransformRule{
            "exclusive_maximum_integer_to_maximum",
            "Setting `exclusiveMaximum` when `type` is `integer` is syntax "
            "sugar for `maximum`"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("type") &&
                     schema.at("type").is_string() &&
                     schema.at("type").to_string() == "integer" &&
                     schema.defines("exclusiveMaximum") &&
                     schema.at("exclusiveMaximum").is_number() &&
                     !schema.defines("maximum"));
    return APPLIES_TO_KEYWORDS("exclusiveMaximum", "type");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("exclusiveMaximum").is_integer()) {
      auto new_maximum = schema.at("exclusiveMaximum");
      new_maximum += sourcemeta::core::JSON{-1};
      schema.at("exclusiveMaximum").into(new_maximum);
      schema.rename("exclusiveMaximum", "maximum");
    } else if (schema.at("exclusiveMaximum").is_decimal()) {
      const auto current{schema.at("exclusiveMaximum").to_decimal()};
      auto new_value{current.to_integral()};
      if (new_value > current) {
        new_value -= sourcemeta::core::Decimal{1};
      }

      if (current.is_integer()) {
        new_value -= sourcemeta::core::Decimal{1};
      }

      if (new_value.is_int64()) {
        schema.at("exclusiveMaximum")
            .into(sourcemeta::core::JSON{new_value.to_int64()});
      } else {
        schema.at("exclusiveMaximum").into(sourcemeta::core::JSON{new_value});
      }

      schema.rename("exclusiveMaximum", "maximum");
    } else {
      const auto current{schema.at("exclusiveMaximum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::floor(current))};
      schema.at("exclusiveMaximum").into(sourcemeta::core::JSON{new_value});
      schema.rename("exclusiveMaximum", "maximum");
    }
  }
};
