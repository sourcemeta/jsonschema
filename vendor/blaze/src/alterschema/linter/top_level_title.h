class TopLevelTitle final : public SchemaTransformRule {
public:
  using mutates = std::false_type;
  using reframe_after_transform = std::false_type;
  TopLevelTitle()
      : SchemaTransformRule{
            "top_level_title",
            "Set a concise non-empty title at the top level of the "
            "schema to explain what the definition is about"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(location.pointer.empty());
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Meta_Data,
         Vocabularies::Known::JSON_Schema_2019_09_Meta_Data,
         Vocabularies::Known::JSON_Schema_Draft_7,
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_3_Hyper,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    const auto *title{schema.try_at("title")};
    if (title && title->is_string() && title->empty()) {
      return APPLIES_TO_KEYWORDS("title");
    } else {
      return !title;
    }
  }
};
