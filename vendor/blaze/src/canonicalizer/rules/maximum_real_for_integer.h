class MaximumRealForInteger final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::false_type;
  MaximumRealForInteger() : SchemaTransformRule{"maximum_real_for_integer"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
             SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}) &&
        schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");
    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number() &&
                     !maximum->is_integral());
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
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
