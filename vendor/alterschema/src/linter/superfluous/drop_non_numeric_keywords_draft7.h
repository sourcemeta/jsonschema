class DropNonNumericKeywords_Draft7 final : public Rule {
public:
  DropNonNumericKeywords_Draft7()
      : Rule{"drop_non_numeric_keywords_draft7",
             "Keywords that don't apply to numbers will never match if the "
             "instance is guaranteed to be a number"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::jsontoolkit::JSON &schema, const std::string &,
            const std::set<std::string> &vocabularies,
            const sourcemeta::jsontoolkit::Pointer &) const -> bool override {
    return vocabularies.contains("http://json-schema.org/draft-07/schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           (schema.at("type").to_string() == "number" ||
            schema.at("type").to_string() == "integer") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(Transformer &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"maxLength",
                                        "minLength",
                                        "pattern",
                                        "contentEncoding",
                                        "contentMediaType",
                                        "additionalItems",
                                        "items",
                                        "contains",
                                        "maxItems",
                                        "minItems",
                                        "uniqueItems",
                                        "maxProperties",
                                        "minProperties",
                                        "required",
                                        "properties",
                                        "patternProperties",
                                        "additionalProperties",
                                        "propertyNames",
                                        "dependencies",
                                        "format"};
};
