class PropertyNamesDefault final : public SchemaTransformRule {
public:
  PropertyNamesDefault()
      : SchemaTransformRule{
            "property_names_default",
            "Setting the `propertyNames` keyword to the empty object "
            "does not add any further constraint"} {};

  [[nodiscard]] auto
  condition(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::JSON &,
            const sourcemeta::core::Vocabularies &vocabularies,
            const sourcemeta::core::SchemaFrame &,
            const sourcemeta::core::SchemaFrame::Location &,
            const sourcemeta::core::SchemaWalker &,
            const sourcemeta::core::SchemaResolver &) const
      -> sourcemeta::core::SchemaTransformRule::Result override {
    ONLY_CONTINUE_IF(
        contains_any(vocabularies,
                     {"https://json-schema.org/draft/2020-12/vocab/applicator",
                      "https://json-schema.org/draft/2019-09/vocab/applicator",
                      "http://json-schema.org/draft-07/schema#",
                      "http://json-schema.org/draft-06/schema#"}) &&
        schema.is_object() && schema.defines("propertyNames") &&
        schema.at("propertyNames").is_object() &&
        schema.at("propertyNames").empty());
    return APPLIES_TO_KEYWORDS("propertyNames");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("propertyNames");
  }
};
