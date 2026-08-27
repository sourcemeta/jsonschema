class TitleTrim final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  TitleTrim()
      : SchemaTransformRule{
            "title_trim",
            "Titles should not contain leading or trailing whitespace"} {};

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
        {SchemaVocabularies::Known::JSON_Schema_2020_12_Meta_Data,
         SchemaVocabularies::Known::JSON_Schema_2019_09_Meta_Data,
         SchemaVocabularies::Known::JSON_Schema_Draft_7,
         SchemaVocabularies::Known::JSON_Schema_Draft_6,
         SchemaVocabularies::Known::JSON_Schema_Draft_4,
         SchemaVocabularies::Known::JSON_Schema_Draft_3,
         SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
         SchemaVocabularies::Known::JSON_Schema_Draft_2,
         SchemaVocabularies::Known::JSON_Schema_Draft_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(schema.defines("title"));
    ONLY_CONTINUE_IF(schema.at("title").is_string());
    ONLY_CONTINUE_IF(!schema.at("title").is_trimmed());
    return APPLIES_TO_KEYWORDS("title");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("title").trim();
  }
};
