class TitleDescriptionEqual final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  TitleDescriptionEqual()
      : SchemaTransformRule{
            "title_description_equal",
            "The title and description metadata keywords should not be set to "
            "the same value"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
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
    ONLY_CONTINUE_IF(schema.defines("title") && schema.defines("description"));
    ONLY_CONTINUE_IF(schema.at("title").is_string() &&
                     schema.at("description").is_string());
    ONLY_CONTINUE_IF(schema.at("title") == schema.at("description"));
    return applies_to_keywords("title", "description");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("description");
  }
};
