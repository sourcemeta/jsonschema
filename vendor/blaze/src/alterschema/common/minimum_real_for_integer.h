class MinimumRealForInteger final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  MinimumRealForInteger()
      : SchemaTransformRule{
            "minimum_real_for_integer",
            "If an instance is guaranteed to be an integer, setting a real "
            "number lower bound is the same as a ceil of that lower bound"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
                          Vocabularies::Known::JSON_Schema_Draft_2,
                          Vocabularies::Known::JSON_Schema_Draft_1}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number() &&
                     !minimum->is_integral());
    return APPLIES_TO_KEYWORDS("minimum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("minimum").is_decimal()) {
      const auto current{schema.at("minimum").to_decimal()};
      auto new_value{current.to_integral()};
      if (new_value < current) {
        new_value += sourcemeta::core::Decimal{1};
      }

      if (new_value.is_int64()) {
        schema.assign("minimum", sourcemeta::core::JSON{new_value.to_int64()});
      } else {
        schema.assign("minimum", sourcemeta::core::JSON{new_value});
      }
    } else {
      const auto current{schema.at("minimum").to_real()};
      const auto new_value{static_cast<std::int64_t>(std::ceil(current))};
      schema.assign("minimum", sourcemeta::core::JSON{new_value});
    }
  }
};
