class ElseWithoutIf final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD{"else"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ElseWithoutIf()
      : SchemaTransformRule{"else_without_if",
                            "The `else` keyword is meaningless "
                            "without the presence of the `if` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     !schema.defines("if"));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer, WeakPointer::Token{std::cref(KEYWORD)}));
    return APPLIES_TO_KEYWORDS(KEYWORD);
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase(KEYWORD);
  }
};
