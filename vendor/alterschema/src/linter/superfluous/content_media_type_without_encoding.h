class ContentMediaTypeWithoutEncoding final : public Rule {
public:
  ContentMediaTypeWithoutEncoding()
      : Rule{"content_media_type_without_encoding",
             "The `contentMediaType` keyword is meaningless "
             "without the presence of the `contentEncoding` keyword"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return contains_any(vocabularies,
                        {"https://json-schema.org/draft/2020-12/vocab/content",
                         "https://json-schema.org/draft/2019-09/vocab/content",
                         "http://json-schema.org/draft-07/schema#"}) &&
           schema.is_object() && schema.defines("contentMediaType") &&
           !schema.defines("contentEncoding");
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase("contentMediaType");
  }
};
