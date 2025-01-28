class DropNonNumericKeywords_Draft4 final : public SchemaTransformRule {
public:
  DropNonNumericKeywords_Draft4()
      : SchemaTransformRule{
            "drop_non_numeric_keywords_draft4",
            "Keywords that don't apply to numbers will never match if the "
            "instance is guaranteed to be a number"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains("http://json-schema.org/draft-04/schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           (schema.at("type").to_string() == "number" ||
            schema.at("type").to_string() == "integer") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"maxLength",
                                        "minLength",
                                        "pattern",
                                        "additionalItems",
                                        "items",
                                        "maxItems",
                                        "minItems",
                                        "uniqueItems",
                                        "maxProperties",
                                        "minProperties",
                                        "required",
                                        "properties",
                                        "patternProperties",
                                        "additionalProperties",
                                        "dependencies",
                                        "format"};
};
