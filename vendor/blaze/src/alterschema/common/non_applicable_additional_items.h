class NonApplicableAdditionalItems final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"additionalItems"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  NonApplicableAdditionalItems()
      : SchemaTransformRule{
            "non_applicable_additional_items",
            "The `additionalItems` keyword is ignored when the "
            "`items` keyword is either not present or set to a schema"} {};

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
            {SchemaVocabularies::Known::JSON_Schema_2019_09_Applicator,
             SchemaVocabularies::Known::JSON_Schema_Draft_7,
             SchemaVocabularies::Known::JSON_Schema_Draft_6,
             SchemaVocabularies::Known::JSON_Schema_Draft_4,
             SchemaVocabularies::Known::JSON_Schema_Draft_3,
             SchemaVocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
        schema.is_object() && schema.defines(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));

    const auto *items{schema.try_at("items")};
    if (items && (items->is_object() || items->is_boolean())) {
      return APPLIES_TO_KEYWORDS(KEYWORD, "items");
    } else if (!items) {
      return APPLIES_TO_KEYWORDS(KEYWORD);
    } else {
      return false;
    }
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
