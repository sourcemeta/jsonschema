class CommentDrop final : public SchemaTransformRule {
public:
  using reframe_after_transform = std::true_type;
  CommentDrop() : SchemaTransformRule{"comment_drop"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &,
            const sourcemeta::blaze::SchemaFrame::Location &,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_2019_09_Core,
                          Vocabularies::Known::JSON_Schema_2020_12_Core}) &&
                     schema.is_object() && schema.defines("$comment"));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase("$comment");
  }
};
