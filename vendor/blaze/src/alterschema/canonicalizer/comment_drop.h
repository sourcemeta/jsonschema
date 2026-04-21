class CommentDrop final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  CommentDrop() : SchemaTransformRule{"comment_drop", ""} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_2019_09_Core,
                          Vocabularies::Known::JSON_Schema_2020_12_Core}) &&
                     schema.is_object() && schema.defines("$comment"));
    return true;
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("$comment");
  }
};
