class ExclusiveMaximumNumberAndMaximum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExclusiveMaximumNumberAndMaximum()
      : SchemaTransformRule{
            "exclusive_maximum_number_and_maximum",
            "Setting both `exclusiveMaximum` and `maximum` at the same time "
            "is considered an anti-pattern. You should choose one"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Validation,
                          Vocabularies::Known::JSON_Schema_2019_09_Validation,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object());

    const auto *maximum{schema.try_at("maximum")};
    ONLY_CONTINUE_IF(maximum && maximum->is_number());
    const auto *exclusive_maximum{schema.try_at("exclusiveMaximum")};
    ONLY_CONTINUE_IF(exclusive_maximum && exclusive_maximum->is_number());
    return APPLIES_TO_KEYWORDS("exclusiveMaximum", "maximum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("maximum") < schema.at("exclusiveMaximum")) {
      schema.erase("exclusiveMaximum");
    } else {
      schema.erase("maximum");
    }
  }
};
