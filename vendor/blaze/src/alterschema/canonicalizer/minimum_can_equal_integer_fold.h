class MinimumCanEqualIntegerFold final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MinimumCanEqualIntegerFold()
      : SchemaTransformRule{"minimum_can_equal_integer_fold", ""} {};

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
        schema.defines("minimumCanEqual") &&
        schema.at("minimumCanEqual").is_boolean() &&
        !schema.at("minimumCanEqual").to_boolean() &&
        schema.defines("minimum") && schema.at("minimum").is_number());
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    const auto &minimum{schema.at("minimum")};
    if (minimum.is_integer()) {
      if (minimum.to_integer() < std::numeric_limits<std::int64_t>::max()) {
        auto new_minimum = minimum;
        new_minimum += sourcemeta::core::JSON{1};
        schema.assign("minimum", std::move(new_minimum));
      } else {
        auto result{
            sourcemeta::core::Decimal{std::to_string(minimum.to_integer())}};
        result += sourcemeta::core::Decimal{1};
        schema.assign("minimum", sourcemeta::core::JSON{std::move(result)});
      }
    } else if (minimum.is_decimal()) {
      auto current{minimum.to_decimal()};
      auto ceiled{current.to_integral()};
      if (ceiled < current) {
        ceiled += sourcemeta::core::Decimal{1};
      }
      if (ceiled == current) {
        ceiled += sourcemeta::core::Decimal{1};
      }
      if (ceiled.is_int64()) {
        schema.assign("minimum", sourcemeta::core::JSON{ceiled.to_int64()});
      } else {
        schema.assign("minimum", sourcemeta::core::JSON{std::move(ceiled)});
      }
    } else {
      const auto value{minimum.to_real()};
      const auto ceil_value{std::ceil(value)};
      if (ceil_value == value) {
        if (value >=
                static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
            value <
                static_cast<double>(std::numeric_limits<std::int64_t>::max()) +
                    1.0) {
          auto decimal_result{sourcemeta::core::Decimal{
              std::to_string(static_cast<std::int64_t>(value))}};
          decimal_result += sourcemeta::core::Decimal{1};
          if (decimal_result.is_int64()) {
            schema.assign("minimum",
                          sourcemeta::core::JSON{decimal_result.to_int64()});
          } else {
            schema.assign("minimum",
                          sourcemeta::core::JSON{std::move(decimal_result)});
          }
        } else {
          auto decimal_fallback{sourcemeta::core::Decimal{value}};
          decimal_fallback += sourcemeta::core::Decimal{1};
          schema.assign("minimum",
                        sourcemeta::core::JSON{std::move(decimal_fallback)});
        }
      } else if (std::isfinite(ceil_value) &&
                 ceil_value >= static_cast<double>(
                                   std::numeric_limits<std::int64_t>::min()) &&
                 ceil_value < static_cast<double>(
                                  std::numeric_limits<std::int64_t>::max()) +
                                  1.0) {
        schema.assign("minimum", sourcemeta::core::JSON{
                                     static_cast<std::int64_t>(ceil_value)});
      } else {
        auto decimal_result{sourcemeta::core::Decimal{value}};
        decimal_result = decimal_result.to_integral();
        if (decimal_result < sourcemeta::core::Decimal{value}) {
          decimal_result += sourcemeta::core::Decimal{1};
        }
        schema.assign("minimum",
                      sourcemeta::core::JSON{std::move(decimal_result)});
      }
    }

    schema.erase("minimumCanEqual");
  }
};
