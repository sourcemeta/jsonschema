class DescriptionTrailingPeriod final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  DescriptionTrailingPeriod()
      : SchemaTransformRule{
            "description_trailing_period",
            "Descriptions should not end with a period to give user interfaces "
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
    ONLY_CONTINUE_IF(schema.defines("description"));
    ONLY_CONTINUE_IF(schema.at("description").is_string());
    const auto &description{schema.at("description").to_string()};
    ONLY_CONTINUE_IF(!description.empty() && description.back() == '.');
    return applies_to_keywords("description");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    auto &description{schema.at("description")};
    auto value{description.to_string()};
    while (!value.empty() && value.back() == '.') {
      value.pop_back();
    }

    description.into(JSON{value});
  }
};
