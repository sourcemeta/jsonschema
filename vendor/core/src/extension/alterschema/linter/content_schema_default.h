class ContentSchemaDefault final : public SchemaTransformRule {
private:
  static inline const std::string KEYWORD{"contentSchema"};

public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ContentSchemaDefault()
      : SchemaTransformRule{
            "content_schema_default",
            "Setting the `contentSchema` keyword to the true schema "
            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaFrame::Location &location,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        vocabularies.contains_any(
            {Vocabularies::Known::JSON_Schema_2020_12_Content,
             Vocabularies::Known::JSON_Schema_2019_09_Content}) &&
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
