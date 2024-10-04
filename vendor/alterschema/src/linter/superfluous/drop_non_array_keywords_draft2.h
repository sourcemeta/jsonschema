class DropNonArrayKeywords_Draft2 final : public Rule {
public:
  DropNonArrayKeywords_Draft2()
      : Rule{"drop_non_array_keywords_draft2",
             "Keywords that don't apply to arrays will never match if the "
             "instance is guaranteed to be an array"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "http://json-schema.org/draft-02/hyper-schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "properties",      "optional",        "additionalProperties",
      "requires",        "minimum",         "maximum",
      "minimumCanEqual", "maximumCanEqual", "pattern",
      "maxLength",       "minLength",       "format",
      "contentEncoding", "divisibleBy"};
};
