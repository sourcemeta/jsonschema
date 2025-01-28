class DropNonNullKeywords_Draft6 final : public SchemaTransformRule {
public:
  DropNonNullKeywords_Draft6()
      : SchemaTransformRule{
            "drop_non_null_keywords_draft6",
            "Keywords that don't apply to null values will never match if the "
            "instance is guaranteed to be null"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains("http://json-schema.org/draft-06/schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "null" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(PointerProxy &transformer) const -> void override {
    transformer.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{"multipleOf",
                                        "maximum",
                                        "exclusiveMaximum",
                                        "minimum",
                                        "exclusiveMinimum",
                                        "maxLength",
                                        "minLength",
                                        "pattern",
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
