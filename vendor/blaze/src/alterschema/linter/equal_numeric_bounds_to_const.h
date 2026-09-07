class EqualNumericBoundsToConst final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  EqualNumericBoundsToConst()
      : SchemaTransformRule{
            "equal_numeric_bounds_to_const",
            "Setting `minimum` and `maximum` to the same number only leaves "
            "one possible value"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any({
            SchemaVocabularies::Known::JSON_SCHEMA_2020_12_VALIDATION,
            SchemaVocabularies::Known::JSON_SCHEMA_2019_09_VALIDATION,
            SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
            SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
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

    return applies_to_keywords("minimum", "maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.rename("minimum", "const");
    schema.erase("type");
    schema.erase("maximum");
  }
};
