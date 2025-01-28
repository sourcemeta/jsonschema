class ContentSchemaWithoutMediaType final : public SchemaTransformRule {
public:
  ContentSchemaWithoutMediaType()
      : SchemaTransformRule{
            "content_schema_without_media_type",
            "The `contentSchema` keyword is meaningless without the presence "
            "of the `contentMediaType` keyword"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/content",
                "https://json-schema.org/draft/2019-09/vocab/content"}) &&
           schema.is_object() && schema.defines("contentSchema") &&
           !schema.defines("contentMediaType");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("contentSchema");
  }
};
