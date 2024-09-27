class DropNonStringKeywords_Draft0 final : public Rule {
public:
  DropNonStringKeywords_Draft0()
      : Rule{"drop_non_string_keywords_draft0",
             "Keywords that don't apply to strings will never match if the "
             "instance is guaranteed to be a string"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return vocabularies.contains(
               "http://json-schema.org/draft-00/hyper-schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "string" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "properties",      "items",    "optional", "additionalProperties",
      "requires",        "minimum",  "maximum",  "minimumCanEqual",
      "maximumCanEqual", "minItems", "maxItems", "maxDecimal"};
};
