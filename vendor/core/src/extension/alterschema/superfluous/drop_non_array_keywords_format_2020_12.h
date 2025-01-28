class DropNonArrayKeywordsFormat_2020_12 final : public SchemaTransformRule {
public:
  DropNonArrayKeywordsFormat_2020_12()
      : SchemaTransformRule{
            "drop_non_array_keywords_format_2020_12",
            "Keywords that don't apply to arrays will never match if the "
            "instance is guaranteed to be an array"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           (vocabularies.contains("https://json-schema.org/draft/2020-12/vocab/"
                                  "format-annotation") ||
            vocabularies.contains("https://json-schema.org/draft/2020-12/vocab/"
                                  "format-assertion")) &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"format"};
};
