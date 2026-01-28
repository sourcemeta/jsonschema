class PatternPropertiesDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  PatternPropertiesDefault()
      : SchemaTransformRule{
            "pattern_properties_default",
            "Setting the `patternProperties` keyword to the empty object "
            "does not add any further constraint"} {};

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
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3}) &&
                     schema.is_object() &&
                     schema.defines("patternProperties") &&
                     schema.at("patternProperties").is_object() &&
                     schema.at("patternProperties").empty());
    return APPLIES_TO_KEYWORDS("patternProperties");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("patternProperties");
  }
};
