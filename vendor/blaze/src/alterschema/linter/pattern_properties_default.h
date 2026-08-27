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
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
        schema.is_object());

    const auto *pattern_properties{schema.try_at("patternProperties")};
    ONLY_CONTINUE_IF(pattern_properties && pattern_properties->is_object() &&
                     pattern_properties->empty());
    return APPLIES_TO_KEYWORDS("patternProperties");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("patternProperties");
  }
};
