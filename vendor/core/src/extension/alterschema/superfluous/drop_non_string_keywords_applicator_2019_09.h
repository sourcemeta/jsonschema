class DropNonStringKeywordsApplicator_2019_09 final
    : public SchemaTransformRule {
public:
  DropNonStringKeywordsApplicator_2019_09()
      : SchemaTransformRule{
            "drop_non_string_keywords_applicator_2019_09",
            "Keywords that don't apply to strings will never match if the "
            "instance is guaranteed to be a string"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "string" &&
           vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/applicator") &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"properties",
                                        "patternProperties",
                                        "additionalProperties",
                                        "dependentSchemas",
                                        "propertyNames",
                                        "additionalItems",
                                        "contains",
                                        "items",
                                        "unevaluatedProperties",
                                        "unevaluatedItems"};
};
