class MultipleOfDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  MultipleOfDefault()
      : SchemaTransformRule{
            "multiple_of_default",
            "Setting `multipleOf` to 1 does not add any further constraint"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object() && schema.defines("multipleOf") &&
                     ((schema.at("multipleOf").is_integer() &&
                       schema.at("multipleOf").to_integer() == 1) ||
                      (schema.at("multipleOf").is_real() &&
                       schema.at("multipleOf").to_real() == 1.0) ||
                      (schema.at("multipleOf").is_decimal() &&
                       schema.at("multipleOf").to_decimal() ==
                           sourcemeta::core::Decimal{1})));
    return APPLIES_TO_KEYWORDS("multipleOf");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("multipleOf");
  }
};
