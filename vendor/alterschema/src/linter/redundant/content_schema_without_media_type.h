class ContentSchemaWithoutMediaType final : public Rule {
public:
  ContentSchemaWithoutMediaType()
      : Rule{"content_schema_without_media_type",
             "The `contentSchema` keyword is meaningless without the presence "
             "of the `contentMediaType` keyword"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return contains_any(
               vocabularies,
               {"https://json-schema.org/draft/2020-12/vocab/content",
                "https://json-schema.org/draft/2019-09/vocab/content"}) &&
           schema.is_object() && schema.defines("contentSchema") &&
           !schema.defines("contentMediaType");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("contentSchema");
  }
};
