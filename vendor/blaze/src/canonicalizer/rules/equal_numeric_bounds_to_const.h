class EqualNumericBoundsToConst final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  EqualNumericBoundsToConst()
      : SchemaTransformRule{"equal_numeric_bounds_to_const"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any({
                         Vocabularies::Known::JSON_Schema_2020_12_Validation,
                         Vocabularies::Known::JSON_Schema_2019_09_Validation,
                         Vocabularies::Known::JSON_Schema_Draft_7,
                         Vocabularies::Known::JSON_Schema_Draft_6,
                     }) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(
        type && type->is_string() &&
        (type->to_string() == "integer" || type->to_string() == "number"));
    const auto *minimum{schema.try_at("minimum")};
    ONLY_CONTINUE_IF(minimum && minimum->is_number());
    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number() && *minimum == *maximum);

    const auto *exclusive_minimum{schema.try_at("exclusiveMinimum")};
    ONLY_CONTINUE_IF(!(exclusive_minimum && exclusive_minimum->is_number() &&
                       *exclusive_minimum >= *minimum));
    const auto *exclusive_maximum{schema.try_at("exclusiveMaximum")};
    ONLY_CONTINUE_IF(!(exclusive_maximum && exclusive_maximum->is_number() &&
                       *exclusive_maximum <= *maximum));

    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.rename("minimum", "const");
    schema.erase("type");
    schema.erase("maximum");
  }
};
