class ExclusiveMinimumIntegerToMinimum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
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
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
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
      if (schema.at("exclusiveMinimum").to_integer() <
          std::numeric_limits<std::int64_t>::max()) {
        auto new_minimum = schema.at("exclusiveMinimum");
        new_minimum += sourcemeta::core::JSON{1};
        schema.at("exclusiveMinimum").into(new_minimum);
      } else {
        auto result{sourcemeta::core::Decimal{
            std::to_string(schema.at("exclusiveMinimum").to_integer())}};
        result += sourcemeta::core::Decimal{1};
        schema.at("exclusiveMinimum")
            .into(sourcemeta::core::JSON{std::move(result)});
      }

      schema.rename("exclusiveMinimum", "minimum");
    } else if (schema.at("exclusiveMinimum").is_decimal()) {
      const auto current{schema.at("exclusiveMinimum").to_decimal()};
      auto new_value{current.to_integral()};
      if (new_value < current) {
        new_value += sourcemeta::core::Decimal{1};
      }

      if (new_value == current) {
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
      const auto ceil_value{std::ceil(current)};
      if (ceil_value == current) {
        auto result{sourcemeta::core::Decimal{current}};
        result += sourcemeta::core::Decimal{1};
        if (result.is_int64()) {
          schema.at("exclusiveMinimum")
              .into(sourcemeta::core::JSON{result.to_int64()});
        } else {
          schema.at("exclusiveMinimum")
              .into(sourcemeta::core::JSON{std::move(result)});
        }
      } else if (std::isfinite(ceil_value) &&
                 ceil_value >= static_cast<double>(
                                   std::numeric_limits<std::int64_t>::min()) &&
                 ceil_value < static_cast<double>(
                                  std::numeric_limits<std::int64_t>::max()) +
                                  1.0) {
        schema.at("exclusiveMinimum")
            .into(
                sourcemeta::core::JSON{static_cast<std::int64_t>(ceil_value)});
      } else {
        auto result{sourcemeta::core::Decimal{current}};
        result = result.to_integral();
        if (result < sourcemeta::core::Decimal{current}) {
          result += sourcemeta::core::Decimal{1};
        }

        schema.at("exclusiveMinimum")
            .into(sourcemeta::core::JSON{std::move(result)});
      }

      schema.rename("exclusiveMinimum", "minimum");
    }
  }
};
