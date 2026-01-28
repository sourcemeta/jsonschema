class ExclusiveMinimumNumberAndMinimum final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ExclusiveMinimumNumberAndMinimum()
      : SchemaTransformRule{
            "exclusive_minimum_number_and_minimum",
            "Setting both `exclusiveMinimum` and `minimum` at the same time "
            "is considered an anti-pattern. You should choose one"} {};

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
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("minimum") &&
                     schema.defines("exclusiveMinimum") &&
                     schema.at("minimum").is_number() &&
                     schema.at("exclusiveMinimum").is_number());
    return APPLIES_TO_KEYWORDS("exclusiveMinimum", "minimum");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    if (schema.at("exclusiveMinimum") < schema.at("minimum")) {
      schema.erase("exclusiveMinimum");
    } else {
      schema.erase("minimum");
    }
  }
};
