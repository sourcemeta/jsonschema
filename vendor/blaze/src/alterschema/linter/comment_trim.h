class CommentTrim final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::false_type;
  CommentTrim()
      : SchemaTransformRule{
            "comment_trim",
            "Comments should not contain leading or trailing whitespace"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &, const bool) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
        {Vocabularies::Known::JSON_Schema_2020_12_Core,
         Vocabularies::Known::JSON_Schema_2019_09_Core,
         Vocabularies::Known::JSON_Schema_Draft_7}));
    ONLY_CONTINUE_IF(schema.is_object());
    ONLY_CONTINUE_IF(schema.defines("$comment"));
    ONLY_CONTINUE_IF(schema.at("$comment").is_string());
    ONLY_CONTINUE_IF(!schema.at("$comment").is_trimmed());
    return APPLIES_TO_KEYWORDS("$comment");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.at("$comment").trim();
  }
};
