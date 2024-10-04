class DropNonArrayKeywordsApplicator_2019_09 final : public Rule {
public:
  DropNonArrayKeywordsApplicator_2019_09()
      : Rule{"drop_non_array_keywords_applicator_2019_09",
             "Keywords that don't apply to arrays will never match if the "
             "instance is guaranteed to be an array"} {};

  [[nodiscard]] auto condition(const sourcemeta::jsontoolkit::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::jsontoolkit::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "array" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/applicator") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "properties",       "patternProperties", "additionalProperties",
      "dependentSchemas", "propertyNames",     "unevaluatedProperties"};
};
