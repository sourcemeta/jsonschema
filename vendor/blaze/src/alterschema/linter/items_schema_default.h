class ItemsSchemaDefault final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"items"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ItemsSchemaDefault()
      : SchemaTransformRule{"items_schema_default",
                            "Setting the `items` keyword to the true schema "
                            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::SchemaVocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {SchemaVocabularies::Known::JSON_Schema_2020_12_Applicator,
             SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper,
             SchemaVocabularies::Known::JSON_Schema_Draft_2,
             SchemaVocabularies::Known::JSON_Schema_Draft_2_Hyper,
             SchemaVocabularies::Known::JSON_Schema_Draft_1,
             SchemaVocabularies::Known::JSON_Schema_Draft_1_Hyper}) &&
        schema.is_object() && schema.defines(KEYWORD) &&
        ((schema.at(KEYWORD).is_boolean() && schema.at(KEYWORD).to_boolean()) ||
         (schema.at(KEYWORD).is_object() && schema.at(KEYWORD).empty())));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
