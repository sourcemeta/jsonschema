class ContentSchemaWithoutMediaType final : public SchemaTransformRule {
private:
  // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
  static inline const std::string KEYWORD{"contentSchema"};

public:
  using reframe_after_transform = std::true_type;
  ContentSchemaWithoutMediaType()
      : SchemaTransformRule{"content_schema_without_media_type"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::blaze::Vocabularies &vocabularies,
            const sourcemeta::blaze::SchemaFrame &frame,
            const sourcemeta::blaze::SchemaFrame::Location &location,
            const sourcemeta::blaze::SchemaWalker &,
            const sourcemeta::blaze::SchemaResolver &) const -> bool override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Content,
                          Vocabularies::Known::JSON_Schema_2019_09_Content}) &&
                     schema.is_object() && schema.defines(KEYWORD) &&
                     !schema.defines("contentMediaType"));
    ONLY_CONTINUE_IF(!frame.has_references_through(
        location.pointer,
        sourcemeta::core::WeakPointer::Token{std::cref(KEYWORD)}));
    return true;
  }

  auto transform(sourcemeta::core::JSON &schema) const -> void override {
    schema.erase(KEYWORD);
  }
};
