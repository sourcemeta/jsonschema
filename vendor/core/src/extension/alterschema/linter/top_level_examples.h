class TopLevelExamples final : public SchemaTransformRule {
public:
  TopLevelExamples()
      : SchemaTransformRule{
            "top_level_examples",
            "Set a non-empty examples array at the top level of the schema to "
            "illustrate the expected data"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(location.pointer.empty());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
         Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6}));
    ONLY_CONTINUE_IF(schema.is_object());
    if (schema.defines("examples") && schema.at("examples").is_array() &&
        schema.at("examples").empty()) {
      return APPLIES_TO_KEYWORDS("examples");
    } else {
      return !schema.defines("examples");
    }
  }
};
