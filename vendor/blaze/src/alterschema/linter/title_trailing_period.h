class TitleTrailingPeriod final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  TitleTrailingPeriod()
      : SchemaTransformRule{
            "title_trailing_period",
            "Titles should not end with a period to give user interfaces "
            "flexibility in presenting the text"} {};

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
    ONLY_CONTINUE_IF(schema.defines("title"));
    ONLY_CONTINUE_IF(schema.at("title").is_string());
    const auto &title{schema.at("title").to_string()};
    ONLY_CONTINUE_IF(!title.empty() && title.back() == '.');
    return applies_to_keywords("title");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto &title{schema.at("title")};
    auto value{title.to_string()};
    while (!value.empty() && value.back() == '.') {
      value.pop_back();
    }

    title.into(JSON{value});
  }
};
