class ContentSchemaWithoutMediaType final : public SchemaTransformRule {
public:
  ContentSchemaWithoutMediaType()
      : SchemaTransformRule{
            "content_schema_without_media_type",
            "The `contentSchema` keyword is meaningless without the presence "
            "of the `contentMediaType` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/content",
                      "https://json-schema.org/draft/2019-09/vocab/content"}) &&
        schema.is_object() && schema.defines("contentSchema") &&
        !schema.defines("contentMediaType"));
    return APPLIES_TO_KEYWORDS("contentSchema");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("contentSchema");
  }
};
