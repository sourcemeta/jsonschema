class DropNonArrayKeywords_Draft2 final : public SchemaTransformRule {
public:
  DropNonArrayKeywords_Draft2()
      : SchemaTransformRule{
            "drop_non_array_keywords_draft2",
            "Keywords that don't apply to arrays will never match if the "
            "instance is guaranteed to be an array"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return vocabularies.contains(
               "http://json-schema.org/draft-02/hyper-schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(JSON &schema) const -> void override {
    schema.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "properties",      "optional",        "additionalProperties",
      "requires",        "minimum",         "maximum",
      "minimumCanEqual", "maximumCanEqual", "pattern",
      "maxLength",       "minLength",       "format",
      "contentEncoding", "divisibleBy"};
};
