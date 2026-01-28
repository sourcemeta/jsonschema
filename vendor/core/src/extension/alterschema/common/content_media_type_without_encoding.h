class ContentMediaTypeWithoutEncoding final : public SchemaTransformRule {
public:
  using mutates = std::true_type;
  using reframe_after_transform = std::true_type;
  ContentMediaTypeWithoutEncoding()
      : SchemaTransformRule{
            "content_media_type_without_encoding",
            "The `contentMediaType` keyword is meaningless "
            "without the presence of the `contentEncoding` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Content,
                          Vocabularies::Known::JSON_Schema_2019_09_Content,
                          Vocabularies::Known::JSON_Schema_Draft_7}) &&
                     schema.is_object() && schema.defines("contentMediaType") &&
                     !schema.defines("contentEncoding"));
    return APPLIES_TO_KEYWORDS("contentMediaType");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("contentMediaType");
  }
};
