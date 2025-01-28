class DropNonObjectKeywords_Draft0 final : public SchemaTransformRule {
public:
  DropNonObjectKeywords_Draft0()
      : SchemaTransformRule{
            "drop_non_object_keywords_draft0",
            "Keywords that don't apply to objects will never match if the "
            "instance is guaranteed to be an object"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "http://json-schema.org/draft-00/hyper-schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "items",           "minimum",  "maximum",         "minimumCanEqual",
      "maximumCanEqual", "minItems", "maxItems",        "maxLength",
      "minLength",       "format",   "contentEncoding", "maxDecimal"};
};
