class DropNonBooleanKeywords_Draft4 final : public SchemaTransformRule {
public:
  DropNonBooleanKeywords_Draft4()
      : SchemaTransformRule{
            "drop_non_boolean_keywords_draft4",
            "Keywords that don't apply to booleans will never match if the "
            "instance is guaranteed to be a boolean"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    return vocabularies.contains("http://json-schema.org/draft-04/schema#") &&
           schema.is_object() && schema.defines("type") &&
           schema.at("type").is_string() &&
           schema.at("type").to_string() == "boolean" &&
           schema.defines_any(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
  }

  auto transform(JSON &schema) const -> void override {
    schema.erase_keys(this->BLACKLIST.cbegin(), this->BLACKLIST.cend());
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
