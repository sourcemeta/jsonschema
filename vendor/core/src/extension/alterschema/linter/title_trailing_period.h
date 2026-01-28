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
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
         Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(schema.defines("title"));
    ONLY_CONTINUE_IF(schema.at("title").is_string());
    const auto &title{schema.at("title").to_string()};
    ONLY_CONTINUE_IF(!title.empty() && title.back() == '.');
    return APPLIES_TO_KEYWORDS("title");
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
