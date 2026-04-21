class MaximumCanEqualIntegerFold final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MaximumCanEqualIntegerFold()
      : SchemaTransformRule{"maximum_can_equal_integer_fold", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({Vocabularies::Known::JSON_Schema_Draft_0,
                                   Vocabularies::Known::JSON_Schema_Draft_1,
                                   Vocabularies::Known::JSON_Schema_Draft_2}) &&
        schema.is_object() && schema.defines("type") &&
        schema.at("type").is_string() &&
        schema.at("type").to_string() == "integer" &&
        schema.defines("maximumCanEqual") &&
        schema.at("maximumCanEqual").is_boolean() &&
        !schema.at("maximumCanEqual").to_boolean() &&
        schema.defines("maximum") && schema.at("maximum").is_number());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const auto &maximum{schema.at("maximum")};
    if (maximum.is_integer()) {
      if (maximum.to_integer() > std::numeric_limits<std::int64_t>::min()) {
        auto new_maximum = maximum;
        new_maximum += sourcemeta::core::JSON{-1};
        schema.assign("maximum", std::move(new_maximum));
      } else {
        auto result{
            sourcemeta::core::Decimal{std::to_string(maximum.to_integer())}};
        result -= sourcemeta::core::Decimal{1};
        schema.assign("maximum", sourcemeta::core::JSON{std::move(result)});
      }
    } else if (maximum.is_decimal()) {
      auto current{maximum.to_decimal()};
      auto floored{current.to_integral()};
      if (floored > current) {
        floored -= sourcemeta::core::Decimal{1};
      }
      if (floored == current) {
        floored -= sourcemeta::core::Decimal{1};
      }
      if (floored.is_int64()) {
        schema.assign("maximum", sourcemeta::core::JSON{floored.to_int64()});
      } else {
        schema.assign("maximum", sourcemeta::core::JSON{std::move(floored)});
      }
    } else {
      const auto value{maximum.to_real()};
      const auto floor_value{std::floor(value)};
      if (floor_value == value) {
        if (value >=
                static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
            value <
                static_cast<double>(std::numeric_limits<std::int64_t>::max()) +
                    1.0) {
          auto decimal_result{sourcemeta::core::Decimal{
              std::to_string(static_cast<std::int64_t>(value))}};
          decimal_result -= sourcemeta::core::Decimal{1};
          if (decimal_result.is_int64()) {
            schema.assign("maximum",
                          sourcemeta::core::JSON{decimal_result.to_int64()});
          } else {
            schema.assign("maximum",
                          sourcemeta::core::JSON{std::move(decimal_result)});
          }
        } else {
          auto decimal_fallback{sourcemeta::core::Decimal{value}};
          decimal_fallback -= sourcemeta::core::Decimal{1};
          schema.assign("maximum",
                        sourcemeta::core::JSON{std::move(decimal_fallback)});
        }
      } else if (std::isfinite(floor_value) &&
                 floor_value >= static_cast<double>(
                                    std::numeric_limits<std::int64_t>::min()) &&
                 floor_value < static_cast<double>(
                                   std::numeric_limits<std::int64_t>::max()) +
                                   1.0) {
        schema.assign("maximum", sourcemeta::core::JSON{
                                     static_cast<std::int64_t>(floor_value)});
      } else {
        auto decimal_result{sourcemeta::core::Decimal{value}};
        decimal_result = decimal_result.to_integral();
        if (decimal_result > sourcemeta::core::Decimal{value}) {
          decimal_result -= sourcemeta::core::Decimal{1};
        }
        schema.assign("maximum",
                      sourcemeta::core::JSON{std::move(decimal_result)});
      }
    }

    schema.erase("maximumCanEqual");
  }
};
