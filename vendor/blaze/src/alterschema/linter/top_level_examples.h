class TopLevelExamples final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  TopLevelExamples()
      : SchemaTransformRule{
            "top_level_examples",
            "Set a non-empty examples array at the top level of the schema to "
            "illustrate the expected data"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::SchemaVocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(!location.parent.has_value());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {SchemaVocabularies::Known::JSON_SCHEMA_2020_12_META_DATA,
         SchemaVocabularies::Known::JSON_SCHEMA_2019_09_META_DATA,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_7,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *examples{schema.try_at("examples")};
    if ((examples != nullptr) && examples->is_array() && examples->empty()) {
      return applies_to_keywords("examples");
    }
    return examples == nullptr;
  }
};
