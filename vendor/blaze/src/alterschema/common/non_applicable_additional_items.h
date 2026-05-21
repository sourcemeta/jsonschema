class NonApplicableAdditionalItems final : public SchemaTransformRule {
private:
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
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6,
                          Vocabularies::Known::JSON_Schema_Draft_4,
                          Vocabularies::Known::JSON_Schema_Draft_3,
                          Vocabularies::Known::JSON_Schema_Draft_3_Hyper}) &&
                     schema.is_object() && schema.defines(KEYWORD));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));

    const auto *items{schema.try_at("items")};
    if (items && is_schema(*items)) {
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
