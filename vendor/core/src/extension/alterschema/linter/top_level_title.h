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
         Vocabularies::Known::JSON_Schema_Draft_6,
         Vocabularies::Known::JSON_Schema_Draft_4,
         Vocabularies::Known::JSON_Schema_Draft_3,
         Vocabularies::Known::JSON_Schema_Draft_2,
         Vocabularies::Known::JSON_Schema_Draft_1}));
    ONLY_CONTINUE_IF(schema.is_object());
    if (schema.defines("title") && schema.at("title").is_string() &&
        schema.at("title").empty()) {
      return APPLIES_TO_KEYWORDS("title");
    } else {
      return !schema.defines("title");
    }
  }
};
