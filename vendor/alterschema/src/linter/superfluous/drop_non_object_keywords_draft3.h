class DropNonObjectKeywords_Draft3 final : public Rule {
public:
  DropNonObjectKeywords_Draft3()
      : Rule{"drop_non_object_keywords_draft3",
             "Keywords that don't apply to objects will never match if the "
             "instance is guaranteed to be an object"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return vocabularies.contains("http://json-schema.org/draft-03/schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "items",     "additionalItems",  "required",         "minimum",
      "maximum",   "exclusiveMinimum", "exclusiveMaximum", "minItems",
      "maxItems",  "uniqueItems",      "pattern",          "minLength",
      "maxLength", "format",           "divisibleBy"};
};
