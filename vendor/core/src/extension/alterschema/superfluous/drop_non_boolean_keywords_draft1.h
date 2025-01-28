class DropNonBooleanKeywords_Draft1 final : public SchemaTransformRule {
public:
  DropNonBooleanKeywords_Draft1()
      : SchemaTransformRule{
            "drop_non_boolean_keywords_draft1",
            "Keywords that don't apply to booleans will never match if the "
            "instance is guaranteed to be a boolean"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "http://json-schema.org/draft-01/hyper-schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "boolean" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "properties",      "items",    "optional",        "additionalProperties",
      "requires",        "minimum",  "maximum",         "minimumCanEqual",
      "maximumCanEqual", "minItems", "maxItems",        "maxLength",
      "minLength",       "format",   "contentEncoding", "maxDecimal"};
};
