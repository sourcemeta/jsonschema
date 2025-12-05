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
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Content,
                          Vocabularies::Known::JSON_Schema_2019_09_Content}) &&
                     schema.is_object() && schema.defines("contentSchema") &&
                     !schema.defines("contentMediaType"));
    return APPLIES_TO_KEYWORDS("contentSchema");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("contentSchema");
  }
};
