class TopLevelDescription final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  TopLevelDescription()
      : SchemaTransformRule{
            "top_level_description",
            "Set a non-empty description at the top level of the "
            "schema to explain what the definition is about in detail"} {};

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
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_6,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_4,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_3_HYPER,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_2,
         SchemaVocabularies::Known::JSON_SCHEMA_DRAFT_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *description{schema.try_at("description")};
    if ((description != nullptr) && description->is_string() &&
        description->empty()) {
      return applies_to_keywords("description");
    }
    return description == nullptr;
  }
};
