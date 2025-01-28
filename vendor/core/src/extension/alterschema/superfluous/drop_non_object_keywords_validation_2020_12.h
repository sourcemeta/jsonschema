class DropNonObjectKeywordsValidation_2020_12 final
    : public SchemaTransformRule {
public:
  DropNonObjectKeywordsValidation_2020_12()
      : SchemaTransformRule{
            "drop_non_object_keywords_validation_2020_12",
            "Keywords that don't apply to objects will never match if the "
            "instance is guaranteed to be an object"} {};

  [[nodiscard]] auto condition(const sourcemeta::core::JSON &schema,
                               const std::string &,
                               const std::set<std::string> &vocabularies,
                               const sourcemeta::core::Pointer &) const
      -> bool override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/validation") &&
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
      "minLength",        "maxLength",  "pattern",          "maximum",
      "exclusiveMinimum", "multipleOf", "exclusiveMaximum", "minimum",
      "minItems",         "maxItems",   "minContains",      "maxContains",
      "uniqueItems"};
};
