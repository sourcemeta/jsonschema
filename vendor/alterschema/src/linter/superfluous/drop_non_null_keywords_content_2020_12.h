class DropNonNullKeywordsContent_2020_12 final : public Rule {
 public:
  DropNonNullKeywordsContent_2020_12()
      : Rule{"drop_non_null_keywords_content_2020_12",
             "Keywords that don't apply to null values will never match if the "
             "instance is guaranteed to be null"} {};

  [[nodiscard]] auto condition(
      const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
      const std::set<std::string> &vocabularies,
      const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.is_object() &&
           ((schema.defines("type") && schema.at("type").is_string() &&
             schema.at("type").to_string() == "null") ||
            (schema.defines("enum") && schema.at("enum").is_array() &&
             every_item_is_null(schema.at("enum").as_array()))) &&
           vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/content") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

 private:
  const std::set<std::string> BLACKLIST{"contentEncoding", "contentMediaType",
                                        "contentSchema"};
};
