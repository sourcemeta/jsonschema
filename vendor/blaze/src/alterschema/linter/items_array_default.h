class ItemsArrayDefault final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ItemsArrayDefault()
      : SchemaTransformRule{"items_array_default",
                            "Setting the `items` keyword to the empty array "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
             SchemaVocabularies::Known::JSON_Schema_Draft_2,
             SchemaVocabularies::Known::JSON_Schema_Draft_2_Hyper,
             SchemaVocabularies::Known::JSON_Schema_Draft_1,
             SchemaVocabularies::Known::JSON_Schema_Draft_1_Hyper}) &&
        schema.is_object());

    const auto *items{schema.try_at("items")};
    ONLY_CONTINUE_IF(items && items->is_array() && items->empty());
    return APPLIES_TO_KEYWORDS("items");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("items");
  }
};
