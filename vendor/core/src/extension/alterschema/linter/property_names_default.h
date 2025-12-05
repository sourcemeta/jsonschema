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
    ONLY_CONTINUE_IF(vocabularies.contains_any(
                         {Vocabularies::Known::JSON_Schema_2020_12_Applicator,
                          Vocabularies::Known::JSON_Schema_2019_09_Applicator,
                          Vocabularies::Known::JSON_Schema_Draft_7,
                          Vocabularies::Known::JSON_Schema_Draft_6}) &&
                     schema.is_object() && schema.defines("propertyNames") &&
                     schema.at("propertyNames").is_object() &&
                     schema.at("propertyNames").empty());
    return APPLIES_TO_KEYWORDS("propertyNames");
  }

  auto transform(JSON &schema, const Result &) const -> void override {
    schema.erase("propertyNames");
  }
};
