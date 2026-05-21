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
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4}) &&
                     schema.is_object());

    const auto *type{schema.try_at("type")};
    ONLY_CONTINUE_IF(type && type->is_string() &&
                     type->to_string() == "integer");

    const auto *multiple_of{schema.try_at("multipleOf")};
    ONLY_CONTINUE_IF(
        multiple_of &&
        ((multiple_of->is_integer() && multiple_of->to_integer() == 1) ||
         (multiple_of->is_real() && multiple_of->to_real() == 1.0) ||
         (multiple_of->is_decimal() &&
          multiple_of->to_decimal() == sourcemeta::core::Decimal{1})));
    return APPLIES_TO_KEYWORDS("multipleOf");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("multipleOf");
  }
};
