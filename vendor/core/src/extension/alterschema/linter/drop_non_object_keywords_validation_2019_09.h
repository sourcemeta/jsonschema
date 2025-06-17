class DropNonObjectKeywordsValidation_2019_09 final
    : public SchemaTransformRule {
public:
  DropNonObjectKeywordsValidation_2019_09()
      : SchemaTransformRule{
            "drop_non_object_keywords_validation_2019_09",
            "Keywords that don't apply to objects will never match if the "
            "instance is guaranteed to be an object"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return vocabularies.contains(
               "https://json-schema.org/draft/2019-09/vocab/validation") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "object" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(JSON &schema) const -> void override {
    schema.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

private:
  const std::set<std::string> BLACKLIST{
      "minLength",        "maxLength",  "pattern",          "maximum",
      "exclusiveMinimum", "multipleOf", "exclusiveMaximum", "minimum",
      "minItems",         "maxItems",   "minContains",      "maxContains",
      "uniqueItems"};
};
