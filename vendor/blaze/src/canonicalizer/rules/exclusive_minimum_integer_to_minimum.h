class ExclusiveMinimumIntegerToMinimum final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  ExclusiveMinimumIntegerToMinimum()
      : SchemaTransformRule{"exclusive_minimum_integer_to_minimum"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && !schema.defines("minimum"));

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    const auto *exclusive_minimum{schema.try_at("exclusiveMinimum")};
    ONLY_CONTINUE_IF(exclusive_minimum && exclusive_minimum->is_number());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
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
