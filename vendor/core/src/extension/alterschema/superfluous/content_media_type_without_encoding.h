class ContentMediaTypeWithoutEncoding final : public SchemaTransformRule {
public:
  ContentMediaTypeWithoutEncoding()
      : SchemaTransformRule{
            "content_media_type_without_encoding",
            "The `contentMediaType` keyword is meaningless "
            "without the presence of the `contentEncoding` keyword"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return contains_any(vocabularies,
                        {"https://json-schema.org/draft/2020-12/vocab/content",
                         "https://json-schema.org/draft/2019-09/vocab/content",
                         "http://json-schema.org/draft-07/schema#"}) &&
           schema.is_object() && schema.defines("contentMediaType") &&
           !schema.defines("contentEncoding");
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase("contentMediaType");
  }
};
